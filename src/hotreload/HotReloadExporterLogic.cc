#include "HotReloadExporterLogic.h"

#include "XGenSplineProcessInputOutput.h"
#include "XGenSplineToCyHair.h"
#include "XGenSplineToXPD.h"

#include <sstream>

#include <ssmath/common_math.h>

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

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};
#endif


#if 0
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

        if (input->hair_format.compare("xpd") == 0) {
            // XPD
            bool ret = XGenSplineToXPD(*input, output);
            (void)ret;
        } else {
            // CyHair
            bool ret = XGenSplineToCyHair(*input, output);
            (void)ret;
        }

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
