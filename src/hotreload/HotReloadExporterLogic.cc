#include "HotReloadExporterLogic.h"

#include "XGenSplineProcessInputOutput.h"
#include "XGenSplineToCyHair.h"

#include <sstream>

#include <ssmath/common_math.h>

#if 0
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

#include <XGen/XgSplineAPI.h>

#include "cyhair-writer.h"

#include <chrono>
#endif



#if defined(GLTF_EXPORTER_SERIALIZER)

// grpc + fb
#include <grpc++/grpc++.h>

#include "hotreload.grpc.fb.h"
#include "hotreload_generated.h"

#endif // GLTF_EXPORTER_SERIALIZER

namespace
{

#if defined(GLTF_EXPORTER_SERIALIZER)

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<grpc::Channel> channel)
    : stub_(Greeter::NewStub(channel)) {}

  std::string SendCyhair(const std::vector<uint8_t> &data) {
    flatbuffers::grpc::MessageBuilder mb;
    auto name_offset = mb.CreateVector(data);
    auto request_offset = CreateCyhairRequest(mb, name_offset);
    mb.Finish(request_offset);
    auto request_msg = mb.ReleaseMessage<CyhairRequest>();

    flatbuffers::grpc::Message<CyhairReply> response_msg;

    grpc::ClientContext context;

    auto status = stub_->SendCyhair(&context, request_msg, &response_msg);
    if (status.ok()) {
      const CyhairReply *response = response_msg.GetRoot();
      return response->message()->str();
    } else {
      std::cerr << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

#if 0
  void SayManyHellos(const std::string &name, int num_greetings,
                     std::function<void(const std::string &)> callback) {
    flatbuffers::grpc::MessageBuilder mb;
    auto name_offset = mb.CreateString(name);
    auto request_offset =
        CreateManyHellosRequest(mb, name_offset, num_greetings);
    mb.Finish(request_offset);
    auto request_msg = mb.ReleaseMessage<ManyHellosRequest>();

    flatbuffers::grpc::Message<HelloReply> response_msg;

    grpc::ClientContext context;

    auto stream = stub_->SayManyHellos(&context, request_msg);
    while (stream->Read(&response_msg)) {
      const HelloReply *response = response_msg.GetRoot();
      callback(response->message()->str());
    }
    auto status = stream->Finish();
    if (!status.ok()) {
      std::cerr << status.error_code() << ": " << status.error_message()
                << std::endl;
      callback("RPC failed");
    }
  }
#endif

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};
#endif


#if 0
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

    // HACK
    static void SendCyhairData(const std::string &cyhair_data)
    {

        std::string server_address("localhost:50051");

        auto channel =
            grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
        GreeterClient greeter(channel);

        // To [ubyte]
        std::vector<uint8_t> buffer;
        buffer.resize(cyhair_data.size());
        memcpy(buffer.data(), cyhair_data.c_str(), buffer.size());

        std::string message = greeter.SendCyhair(buffer);
        std::cerr << "SendCyhair received: " << message << std::endl;
    }
#endif

} // namespace

Shared
{
    // Write your own function here.
    DLLExport void exportFunc(const void* in_arg, void* out_arg)
    {
        const XGenSplineProcessInput* input = reinterpret_cast<const XGenSplineProcessInput*>(in_arg);
        XGenSplineProcessOutput* output = reinterpret_cast<XGenSplineProcessOutput*>(out_arg);

        bool ret = XGenSplineToCyHair(*input, output);
        (void)ret;

#if 0
        //
        // HACK
        //
        {
            const auto start_time = std::chrono::system_clock::now();

            SendCyhairData(output->cyhair_data);

            const auto end_time = std::chrono::system_clock::now();
            std::chrono::duration<double, std::milli> ms = end_time - start_time;
        }

        MGlobal::displayInfo("Strand conversion time: " + MString(duration.c_str()) + " [ms]");
#endif

        return;
    }
}
