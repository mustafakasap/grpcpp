#ifndef GRPC_CLIENT_H
#define GRPC_CLIENT_H

#include <grpc/grpc.h>
#include <grpcpp/channel.h>

#include <memory>
#include <thread>

#include "client_server_bidi_async_stream.grpc.pb.h"

#include "log_helper.h"

using namespace client_server::grpc::v1;

class GClient {
  public:
    GClient(const GClient &) = delete;
    GClient(GClient &&) = delete;
    GClient &operator=(const GClient &) = delete;
    GClient &operator=(GClient &&) = delete;

    static GClient &GetInstance() {
        static GClient kGlobalInstance; // thread safe only in C++11
        return kGlobalInstance;
    }

    bool Initialize(const std::string &server);
    void Run();
    void Stop();

  private:
    int64_t m_seq_num = 1;

    std::atomic_bool m_running{false};
    std::unique_ptr<CSService01::Stub> m_stub;
    ::grpc::ClientContext m_context{};
    std::unique_ptr<::grpc::ClientReaderWriter<CSService01Method01MessageIn01, CSService01Method01MessageOut01>> m_client_reader_writer_stream{};

    std::thread thread_make_server_request;
    std::thread thread_handle_server_response;

    void handle_server_response();
    void make_server_request();

    GClient() = default;
    virtual ~GClient() = default;
};

#endif  // GRPC_CLIENT_H