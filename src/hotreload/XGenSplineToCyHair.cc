#include "XGenSplineToCyHair.h"
#include "cyhair-writer.h"

#include <chrono>
#include <map>
#include <set>
#include <sstream>

#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnPluginData.h>
#include <maya/MGlobal.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPoint.h>
#include <maya/MPxData.h>
#include <maya/MRampAttribute.h>
#include <maya/MString.h>

// For XgUtil
#ifdef _MSC_VER
#define OSWin_
#endif

#include <XGen/XgSplineAPI.h>
#include <XGen/XgUtil.h> // XGError

namespace
{
    enum RampInterpolation
    {
        RAMP_INTERPOLATION_NONE = 0,
        RAMP_INTERPOLATION_LINEAR = 1,
        RAMP_INTERPOLATION_SMOOTH = 2,
        RAMP_INTERPOLATION_SPLINE = 3
    };

    struct RampParameter
    {
        int index = 0;
        float position = 0.0f;
        float value = 1.0f;
        RampInterpolation interpolation = RAMP_INTERPOLATION_LINEAR;
    };

    struct WidthParameter
    {
        float width;           // `Width Scale` in Attribute editor UI
        float widthTaper;      // `Taper`
        float widthTaperStart; // `Taper Start`

        std::vector<RampParameter> widthRamps;
    };

    ///
    /// Get a material(ShadingGroup in Maya) assigned to XGen node.
    ///
    static MObject GetMaterialOfXGenNode(const MDagPath& dagPath)
    {

        MStatus status = MStatus::kSuccess;

        MDagPath shapePath(dagPath);
        shapePath.extendToShape();

        MFnDagNode node(shapePath, &status);
        if (MStatus::kSuccess != status)
        {
            return MObject::kNullObj;
        }

        MObjectArray sgs; // shading groups
        MObjectArray compos;
        node.getConnectedSetsAndMembers(shapePath.instanceNumber(), sgs, compos, true);

        if (sgs.length() < 1)
        {
            // No material assigned?
            return MObject::kNullObj;
        }

        MFnDependencyNode sg_fn(sgs[0]);

        MGlobal::displayInfo("SG: " + sg_fn.name());

        MPlug shaderPlug = sg_fn.findPlug("surfaceShader");
        if (!shaderPlug.isNull())
        {
            MPlugArray connectedPlugs;
            bool asSrc = false;
            bool asDst = true;
            shaderPlug.connectedTo(connectedPlugs, asDst, asSrc);
            if (connectedPlugs.length() == 1)
            {
                // Return SG, not surfaceShader, since glTFExporter looks up `surfaceShader` when converting Maya shader to Material.
                return sgs[0];
            }
            else
            {
                // Fail to get a shader for some reason(link invalid?)
            }
        }

        return MObject::kNullObj;
    }

    ///
    /// Extract width parameter
    /// width parameter is stored in shape node(e.g. `description1_Shape.width`)
    ///
    static bool ExtractWidthParameter(const MDagPath& dagPath, WidthParameter* output)
    {
        MStatus status;

        MFnDagNode node(dagPath, &status);
        if (MStatus::kSuccess != status)
        {
            return false;
        }

        output->widthRamps.clear();

        // width
        {
            MPlug width = node.findPlug("width", &status);
            if (MStatus::kSuccess == status)
            {
                output->width = width.asFloat();
                std::cerr << "width = " << output->width << std::endl;
            }
        }

        // taper
        {
            MPlug taper = node.findPlug("widthTaper", &status);
            if (MStatus::kSuccess == status)
            {
                output->widthTaper = taper.asFloat();
                std::cerr << "taper = " << output->widthTaper << std::endl;
            }
        }

        // taper start
        {
            MPlug taperStart = node.findPlug("widthTaperStart", &status);
            if (MStatus::kSuccess == status)
            {
                output->widthTaperStart = taperStart.asFloat();
                std::cerr << "taper start = " << output->widthTaperStart << std::endl;
            }
        }

        // width ramp
        {
            MPlug mp = node.findPlug("widthRamp", &status);

            MRampAttribute ramp(mp, &status);
            if (MStatus::kSuccess == status)
            {
                std::cerr << "nument = " << ramp.getNumEntries() << std::endl;
                MIntArray indexes;
                MFloatArray positions;
                MFloatArray values;
                MIntArray interps;
                ramp.getEntries(indexes, positions, values, interps);

                std::cerr << "positions.len = " << positions.length() << std::endl;

                for (int k = 0; k < positions.length(); k++)
                {

                    RampParameter param;
                    // NOTE(LTE): Usually k == indexes[k]
                    param.index = indexes[k];
                    param.position = positions[k];
                    param.value = values[k];
                    param.interpolation = static_cast<RampInterpolation>(interps[k]);

                    output->widthRamps.push_back(param);
                }
            }
        }

        return true;
    }

    ///
    /// Extract opaque binary plugin data(spline data).
    ///
    static bool ExtractPluginData(const MDagPath& dagPath, std::stringstream* oss)
    {
        // There is a `outRenderData` for XGen shape node.
        // e.g. `description1_Shape.outRenderData`

        MStatus status;

        MFnDagNode node(dagPath, &status);
        if (MStatus::kSuccess != status)
        {
            return false;
        }

        MPlug outRenderDataPlug = node.findPlug("outRenderData");
        if (MStatus::kSuccess != status)
        {
            return false;
        }

        MObject obj = outRenderDataPlug.asMObject();
        MPxData* px_data = MFnPluginData(obj).data();

        if (px_data)
        {
            px_data->writeBinary(*oss);

            if (oss->str().size() < 4)
            {
                // Data size too short
                return false;
            }

            return true;
        }

        return false;
    }

} // namespace

bool XGenSplineToCyHair(const XGenSplineProcessInput& input, XGenSplineProcessOutput* output)
{
    const int num_strands = input.num_strands; // -1 = export all strands.
    const bool phantom_points = input.phantom_points;
    const bool cv_repeat = input.cv_repeat;
    std::cerr << "phantom points = " << phantom_points << std::endl;
    std::cerr << "CV repeat = " << cv_repeat << std::endl;

    const MDagPath dag = input.dagPath;
    MString fullPathName = dag.fullPathName();

    MGlobal::displayInfo("Exporting " + fullPathName);

    const char* str = fullPathName.asChar();

    WidthParameter width_parameter;
    {
        bool ret = ExtractWidthParameter(dag, &width_parameter);
        if (!ret)
        {
            MGlobal::displayError("Failed to extract XGen width parameter. dag = " + fullPathName);
            return false;
        }
    }

    std::stringstream binary_data;
    {
        bool ret = ExtractPluginData(dag, &binary_data);
        if (!ret)
        {
            MGlobal::displayError("Failed to extract XGen spline data. dag = " + fullPathName);
            return false;
        }
    }

    std::cout << "binary size = " + std::to_string(binary_data.str().size()) << "\n";

    XGenSplineAPI::XgFnSpline _splines;
    const float sample_time = 0.0f; // TODO(LTE)

    const auto start_time = std::chrono::system_clock::now();

    if (!_splines.load(binary_data, binary_data.str().size(), sample_time))
    {
        MGlobal::displayError("Failed to load XGen spline data. dag = " + fullPathName);
        return false;
    }

    // Execute script to remove culled primitives from primitiveInfos array
    _splines.executeScript();

    std::set<std::string> meshIds;
    for (XGenSplineAPI::XgItSpline splineIt = _splines.iterator(); !splineIt.isDone(); splineIt.next())
    {
        const std::string meshId = splineIt.boundMeshId();

        meshIds.insert(meshId);
    }

    size_t meshNum = meshIds.size();
    if (meshNum == 0)
    {
        XGError("No spline data found from plug: " + std::string(fullPathName.asChar()));
        return false;
    }

    MGlobal::displayInfo("Loaded spline data. dag = " + fullPathName);

    MObject shaderObject = GetMaterialOfXGenNode(dag);
    output->shader = shaderObject;

    XGenSplineAPI::XgItSpline it = _splines.iterator();

    // TODO(LTE): Create array for each spline primitive and do not create 1D global array.
    // std::vector<float> patch_uvs; // TODO(LTE)

    // Reference.
    std::vector<float>& texcoords = output->texcoords;
    std::vector<float>& points = output->points;
    std::vector<float>& radiuss = output->radiuss;
    std::vector<uint32_t>& num_points = output->num_points;

    size_t counts = 0; // exported number of strands;
    for (; !it.isDone(); it.next())
    {
        const uint32_t stride = it.primitiveInfoStride();

        const uint32_t primitiveCount = it.primitiveCount();
        const uint32_t* primitiveInfos = it.primitiveInfos();

        int motion = 0; // TODO(LTE): Support motion time

        const SgVec3f* in_positions = it.positions(motion);
        const float* in_widths = it.width(motion);
        const SgVec2f* in_texcoords = it.texcoords(motion);
        const SgVec2f* in_patchUVs = it.patchUVs(motion);
        const SgVec3f* in_widthDirection = it.widthDirection(motion);

        for (uint32_t i = 0; i < primitiveCount; i++, counts++)
        {
            // Up to num_strands
            if (num_strands > 0)
            {
                if (counts >= num_strands)
                {
                    break;
                }
            }

            const uint32_t offset = primitiveInfos[i * stride];
            const uint32_t length = primitiveInfos[i * stride + 1];

            if (phantom_points)
            {
                num_points.push_back(
                    length + 2); // +2 = phantom points
            }
            else if (cv_repeat)
            {
                num_points.push_back(
                    length + 2 + 2); // +2 for the first, another +2 for the last
            }
            else
            {
                num_points.push_back(
                    length);
            }

            // add phantom points at the beginning.
            if (phantom_points)
            {
                points.push_back(in_positions[offset][0] + (in_positions[offset][0] - in_positions[offset + 1][0]));
                points.push_back(in_positions[offset][1] + (in_positions[offset][1] - in_positions[offset + 1][1]));
                points.push_back(in_positions[offset][2] + (in_positions[offset][2] - in_positions[offset + 1][2]));

                radiuss.push_back(0.5f * (in_widths[offset] + (in_widths[offset] - in_widths[offset + 1])));

                texcoords.push_back(in_texcoords[offset][0] + (in_texcoords[offset][0] - in_texcoords[offset + 1][0]));
                texcoords.push_back(in_texcoords[offset][1] + (in_texcoords[offset][1] - in_texcoords[offset + 1][1]));
            }
            else if (cv_repeat)
            {
                // http://www.fundza.com/mtor/maya_curves_prman/add_attribute/index.html
                // repet the first point two times.
                points.push_back(in_positions[offset][0]);
                points.push_back(in_positions[offset][1]);
                points.push_back(in_positions[offset][2]);

                points.push_back(in_positions[offset][0]);
                points.push_back(in_positions[offset][1]);
                points.push_back(in_positions[offset][2]);

                radiuss.push_back(0.5f * in_widths[offset]);
                radiuss.push_back(0.5f * in_widths[offset]);

                texcoords.push_back(in_texcoords[offset][0]);
                texcoords.push_back(in_texcoords[offset][1]);

                texcoords.push_back(in_texcoords[offset][0]);
                texcoords.push_back(in_texcoords[offset][1]);
            }

            // For strand CVs.
            for (uint32_t k = 0; k < length; k++)
            {
                const size_t idx = offset + k;
                points.push_back(in_positions[idx][0]);
                points.push_back(in_positions[idx][1]);
                points.push_back(in_positions[idx][2]);
                //std::cout << "p = " << positions[idx][0] << ", " << positions[idx][1] << ", " << positions[idx][2] << std::endl;

                radiuss.push_back(in_widths[idx] * 0.5f);
                //std::cout << "width[" << idx << "] = " << widths[idx] << std::endl;
                texcoords.push_back(in_texcoords[offset + k][0]);
                texcoords.push_back(in_texcoords[offset + k][1]);

                //std::cout << "uvs[" << idx << "] = " << patchUVs[idx][0] << ", " << patchUVs[idx][1] << std::endl;

                // DBG:
                // std::cout << "widthDir[" << idx << "] = " << in_widthDirection[offset + k][0] << ", " << in_widthDirection[offset + k][1] << ", " << in_widthDirection[offset + k][2] << "]\n";
            }

            // add phantom points at the end.
            if (phantom_points)
            {
                size_t idx = offset + length - 1;
                points.push_back(in_positions[idx][0] + (in_positions[idx][0] - in_positions[idx - 1][0]));
                points.push_back(in_positions[idx][1] + (in_positions[idx][1] - in_positions[idx - 1][1]));
                points.push_back(in_positions[idx][2] + (in_positions[idx][2] - in_positions[idx - 1][2]));

                radiuss.push_back(0.5f * (in_widths[idx] + (in_widths[idx] - in_widths[idx - 1])));

                texcoords.push_back(in_texcoords[idx][0] + (in_texcoords[idx][0] - in_texcoords[idx - 1][0]));
                texcoords.push_back(in_texcoords[idx][1] + (in_texcoords[idx][1] - in_texcoords[idx - 1][1]));
            }
            else if (cv_repeat)
            {
                // repet the first point two times.
                points.push_back(in_positions[offset][0]);
                points.push_back(in_positions[offset][1]);
                points.push_back(in_positions[offset][2]);

                points.push_back(in_positions[offset][0]);
                points.push_back(in_positions[offset][1]);
                points.push_back(in_positions[offset][2]);

                radiuss.push_back(0.5f * in_widths[offset]);

                radiuss.push_back(0.5f * in_widths[offset]);

                texcoords.push_back(in_texcoords[offset][0]);
                texcoords.push_back(in_texcoords[offset][1]);

                texcoords.push_back(in_texcoords[offset][0]);
                texcoords.push_back(in_texcoords[offset][1]);
            }
        }
    }

    // Serialize to CyHair format.
    output->cyhair_data = cyhair_writer::SerializeAsCyHair(points, radiuss, texcoords, num_points, /* export radius */ true, /* export texcoords */ true);

    const auto end_time = std::chrono::system_clock::now();
    std::chrono::duration<double, std::milli> ms = end_time - start_time;

    std::string duration = std::to_string(ms.count());
    {
        std::stringstream ss;
        ss << "Converted " << counts << " splines";
        MString msg(ss.str().c_str());
        MGlobal::displayInfo(msg);
    }

    {
        std::stringstream ss;
        ss << "CyHair data size: " << output->cyhair_data.size() << " bytes";
        MString msg(ss.str().c_str());
        MGlobal::displayInfo(msg);
    }
    return true;
}
