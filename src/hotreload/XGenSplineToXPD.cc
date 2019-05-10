#include "XGenSplineToXPD.h"

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

#include <XGen/XgSplineAPI.h>
#include <XGen/XgUtil.h>

#include <xpd/Xpd.h>

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

    // From XgSplineDataToXpdCmd.cpp
    static std::string makeNameUnique(const MString& output, const std::string& meshId)
    {
        std::string finalName;

        std::string convertedMeshId(meshId);
        for (size_t i = 0; i < meshId.size(); i++)
        {
            if (meshId[i] == '.' || meshId[i] == '[' || meshId[i] == ']')
                convertedMeshId[i] = '_';
        }

        int dotPos = output.rindex('.');
        int dirSepratorPos = output.rindex('/');
        int dirSepratorPos2 = output.rindex('\\');
        if (dirSepratorPos < dirSepratorPos2)
            dirSepratorPos = dirSepratorPos2;

        if (dotPos < 0 || dotPos < dirSepratorPos)
        {
            // no file extension, append suffix to last
            finalName = std::string(output.asChar()) + convertedMeshId;
        }
        else
        {
            finalName = std::string(output.substring(0, dotPos - 1).asChar()) + convertedMeshId + std::string(output.substring(dotPos, output.length() - 1).asChar());
        }

        return finalName;
    }

} // namespace

bool XGenSplineToXPD(const XGenSplineProcessInput& input, XGenSplineProcessOutput* output)
{
    // TODO(LTE): Read export parameters
    const int num_strands = input.num_strands; // -1 = export all strands.
    //const bool phantom_points = input.phantom_points;
    //const bool cv_repeat = input.cv_repeat;

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
        std::cerr << "meshId = " << meshId << std::endl;

        meshIds.insert(meshId);
    }

    size_t meshNum = meshIds.size();
    std::cerr << "meshNum = " << meshNum << std::endl;
    if (meshNum == 0)
    {
        XGError("No spline data found from : " + std::string(fullPathName.asChar()));
        return false;
    }

    MGlobal::displayInfo("Loaded spline data. dag = " + fullPathName);

    MObject shaderObject = GetMaterialOfXGenNode(dag);
    output->shader = shaderObject;

    MString output_name = dag.fullPathName(); // FIXME(LTE):

    // for each meshId
    for (const std::string& meshId : meshIds)
    {
        std::string xpdFilename;
        if (meshNum > 1)
        {
            // Append suffix to the output filename
            xpdFilename = makeNameUnique(output_name, meshId);
        }
        else
        {
            xpdFilename = output_name.asChar();
        }

        std::map<unsigned int, std::vector<std::vector<unsigned int> > > faceToDataMap;
        unsigned int primCount = 0; // accumulated
        unsigned int maxFaceId = 0;
        unsigned int curItemNum = 1;
        unsigned int cvCountPerPrim = 5; // this will be filled in splineIt iterator loop and also referenced in subsequent stage.

        std::vector<const SgVec2f*> uvArray;
        std::vector<const SgVec3f*> posArray;
        std::vector<const float*> widthArray;
        std::vector<const SgVec3f*> widthDirArray;

        for (XGenSplineAPI::XgItSpline splineIt = _splines.iterator(); !splineIt.isDone(); splineIt.next())
        {
            // Process splines with same meshId
            if (splineIt.boundMeshId() != meshId)
            {
                continue;
            }

            for (auto it : faceToDataMap)
            {
                std::vector<unsigned int> emptyArr;
                it.second.push_back(emptyArr);
            }

            const unsigned int stride = splineIt.primitiveInfoStride();
            primCount += splineIt.primitiveCount();
            const unsigned int* primitiveInfos = splineIt.primitiveInfos();
            cvCountPerPrim = primitiveInfos[1]; // each prim has same cv count

            const unsigned int* faceId = splineIt.faceId();

            for (unsigned int p = 0; p < splineIt.primitiveCount(); p++)
            {
                const unsigned int offset = primitiveInfos[p * stride] / cvCountPerPrim;
                auto it = faceToDataMap.find(faceId[offset]);
                if (faceId[offset] > maxFaceId)
                    maxFaceId = faceId[offset];

                if (it == faceToDataMap.end())
                {
                    std::vector<std::vector<unsigned int> > dataIndex;
                    dataIndex.resize(curItemNum);
                    dataIndex.back().push_back(offset);
                    faceToDataMap.insert(std::make_pair(faceId[offset], dataIndex));
                }
                else
                {
                    std::vector<std::vector<unsigned int> >& data = it->second;
                    if (data.size() < curItemNum)
                    {
                        data.resize(curItemNum);
                    }
                    (it->second).back().push_back(offset);
                }
            }

            uvArray.push_back(splineIt.faceUV());
            posArray.push_back(splineIt.positions());
            widthArray.push_back(splineIt.width());
            widthDirArray.push_back(splineIt.widthDirection());

            curItemNum++;

        } // splineIt

        // Write XPD
        // TODO(LTE): Use TinyXPD instead of XpdWriter

        safevector<std::string> keys; // empty
        safevector<std::string> blocks;
        blocks.push_back("BakedGroom");

        constexpr int PRIM_ATTR_VERSION = 3;
        XpdWriter* xFile = XpdWriter::open(xpdFilename, maxFaceId + 1,
                                           Xpd::Spline, PRIM_ATTR_VERSION,
                                           Xpd::Object, blocks,
                                           float(sample_time),
                                           cvCountPerPrim, &keys);

        // iterate each face
        for (unsigned int faceId = 0; faceId <= maxFaceId; faceId++)
        {
            auto it = faceToDataMap.find(faceId);
            unsigned int id = 0;

            // for face without primitive, still start a face
            if (!xFile->startFace(faceId))
            {
                XGError("Failed to start a new face in XPD file: " + xpdFilename);
            }
            else if (!xFile->startBlock())
            {
                XGError("Failed to start block in XPD file: " + xpdFilename);
            }

            // fill primitives data
            if (it != faceToDataMap.end())
            {
                auto& data = it->second;
                for (size_t i = 0; i < data.size(); i++)
                {
                    std::vector<unsigned int>& prims = data[i];
                    for (unsigned int index : prims)
                    {
                        safevector<float> primData;
                        primData.push_back((float)id++);
                        primData.push_back(uvArray[i][index][0]);
                        primData.push_back(uvArray[i][index][1]);

                        unsigned int posOffset = index * cvCountPerPrim;
                        for (unsigned int j = 0; j < cvCountPerPrim; j++)
                        {
                            primData.push_back((float)posArray[i][posOffset][0]);
                            primData.push_back((float)posArray[i][posOffset][1]);
                            primData.push_back((float)posArray[i][posOffset][2]);
                            posOffset++;
                        }

                        // cv attr
                        primData.push_back(1.0);                                         // length. FIXME(LTE):
                        primData.push_back(widthArray[i][index * cvCountPerPrim]);       // width
                        primData.push_back(width_parameter.widthTaper);                  // taper
                        primData.push_back(width_parameter.widthTaperStart);             // taper start
                        primData.push_back(widthDirArray[i][index * cvCountPerPrim][0]); // width vector .x
                        primData.push_back(widthDirArray[i][index * cvCountPerPrim][1]); // width vector .y
                        primData.push_back(widthDirArray[i][index * cvCountPerPrim][2]); // width vector .z

                        xFile->writePrim(primData);
                    }
                }
            }
        }

        xFile->close();

    } // meshId

    return true;
}
