#include "HotReloadExporterLogic.h"

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
#include <maya/MString.h>

#include <ssmath/common_math.h>

#include <XGen/XgSplineAPI.h>

#include <chrono>

#include "XGenHairProcessInputOutput.h"

namespace
{

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

        if (sgs.length() < 1) {
            // No material assigned?
            return MObject::kNullObj;
        }

        MFnDependencyNode sg_fn(sgs[0]);

        MGlobal::displayInfo("SG: " + sg_fn.name());

        MPlug shaderPlug = sg_fn.findPlug("surfaceShader");
        if (!shaderPlug.isNull()) {
            MPlugArray connectedPlugs;
            bool asSrc = false;
            bool asDst = true;
            shaderPlug.connectedTo( connectedPlugs, asDst, asSrc );
            if (connectedPlugs.length() == 1) {
                // Return SG, not surfaceShader, since glTFExporter looks up `surfaceShader` when converting Maya shader to Material.
                return sgs[0];
            } else {
                // Fail to get a shader for some reason(link invalid?)
            }
        }

        return MObject::kNullObj;
    }

    ///
    /// Extract opaque binary plugin data.
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

Shared
{
    // Write your own function here.
    DLLExport void exportFunc(const void* in_arg, void *out_arg)
    {
        const XGenHairProcessInput *input = reinterpret_cast<const XGenHairProcessInput *>(in_arg);
        XGenHairProcessOutput *output = reinterpret_cast<XGenHairProcessOutput *>(out_arg);

        // TODO(LTE): Read export parameters
        const int num_strands = -1; // -1 = export all strands.
        const bool phantom_points = false;
        const bool cv_repeat = true;

        const MDagPath dag = input->dagPath;
        MString fullPathName = dag.fullPathName();

        MGlobal::displayInfo("Exporting " + fullPathName);

        const char* str = fullPathName.asChar();

        std::stringstream binary_data;
        {
            bool ret = ExtractPluginData(dag, &binary_data);
            if (!ret)
            {
                MGlobal::displayError("Failed to extract XGen spline data. dag = " + fullPathName);
                return;
            }
        }

        std::cout << "binary size = " + std::to_string(binary_data.str().size()) << "\n";

        XGenSplineAPI::XgFnSpline _splines;
        const float sample_time = 0.0f; // TODO(LTE)

        const auto start_time = std::chrono::system_clock::now();

        if (!_splines.load(binary_data, binary_data.str().size(), sample_time))
        {
            MGlobal::displayError("Failed to load XGen spline data. dag = " + fullPathName);
            return;
        }

        MGlobal::displayInfo("Loaded spline data. dag = " + fullPathName);

        MObject shaderObject = GetMaterialOfXGenNode(dag);
        output->shader = shaderObject;

        XGenSplineAPI::XgItSpline it = _splines.iterator();

        // TODO(LTE): Create array for each spline primitive and do not create 1D global array.
        // std::vector<float> patch_uvs; // TODO(LTE)
        std::vector<float> &texcoords = output->texcoords;
        std::vector<float> &points = output->points;
        std::vector<float> &radiuss = output->radiuss;
        std::vector<uint32_t> &num_points = output->num_points;

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

        const auto end_time = std::chrono::system_clock::now();
        std::chrono::duration<double, std::milli> ms = end_time - start_time;

        std::string duration = std::to_string(ms.count());
        std::stringstream ss;
        ss << "Converted " << counts << " splines";
        MString msg(ss.str().c_str());
        MGlobal::displayInfo(msg);
        MGlobal::displayInfo("Strand conversion time: " + MString(duration.c_str()) + " [ms]");

        return;
    }
}
