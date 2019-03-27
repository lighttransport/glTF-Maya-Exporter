// Dummy server implementation for gRPC + flatbuffers testing.

#if defined(_WIN32)
// required for grpc
#define _WIN32_WINNT 0x600
#endif

#include "hotreload.grpc.fb.h"
#include "hotreload_generated.h"

#include <grpc++/grpc++.h>

#include <iostream>
#include <memory>
#include <string>

class GreeterServiceImpl final : public Greeter::Service
{
    virtual grpc::Status SendCyhair(
        grpc::ServerContext* context,
        const flatbuffers::grpc::Message<CyhairRequest>* request_msg,
        flatbuffers::grpc::Message<CyhairReply>* response_msg) override
    {
        // flatbuffers::grpc::MessageBuilder mb_;
        // We call GetRoot to "parse" the message. Verification is already
        // performed by default. See the notes below for more details.
        const CyhairRequest* request = request_msg->GetRoot();
        // Fields are retrieved as usual with FlatBuffers
        const flatbuffers::Vector<uint8_t>* data = request->data();
        // `flatbuffers::grpc::MessageBuilder` is a `FlatBufferBuilder` with a
        // special allocator for efficient gRPC buffer transfer, but otherwise
        // usage is the same as usual.
        auto msg_offset = mb_.CreateString("Resp, " + std::to_string(data->size()));
        auto hello_offset = CreateCyhairReply(mb_, msg_offset);
        mb_.Finish(hello_offset);
        // The `ReleaseMessage<T>()` function detaches the message from the
        // builder, so we can transfer the resopnse to gRPC while simultaneously
        // detaching that memory buffer from the builer.
        *response_msg = mb_.ReleaseMessage<CyhairReply>();
        assert(response_msg->Verify());
        // Return an OK status.
        return grpc::Status::OK;
    }

    flatbuffers::grpc::MessageBuilder mb_;
};

void RunServer(const int port)
{
    std::string server_address = "0.0.0.0:" + std::to_string(port);

    GreeterServiceImpl service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cerr << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, const char* argv[])
{

    int port = 50051;

    if (argc > 1)
    {
        port = atoi(argv[1]);
    }

    RunServer(port);
    return 0;
}
