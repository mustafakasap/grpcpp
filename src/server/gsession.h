#ifndef GRPC_SESSION_H
#define GRPC_SESSION_H

#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/server_context.h>

#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "grpctype.h"
#include "greeter.grpc.pb.h"

using namespace client_server::grpc::v1;

class GSession {
  public:
    std::mutex m_mutex{};
    GrpcSessionStatus m_status{GrpcSessionStatus::WAIT_CONNECT};
    ::grpc::ServerContext m_server_context{};

    GSession(uint64_t session_id) : m_session_id(session_id){};

    bool Initialize();
    void Process(GrpcEvent event);
    void Reply();
    void Finish();

  private:
    uint64_t m_seq_num = -10;

    const uint64_t m_session_id{0};
    ::grpc::ServerAsyncReaderWriter<HelloReply, HelloRequest> m_subscribe_stream{&m_server_context};

    HelloRequest m_request{};
    std::deque<std::shared_ptr<HelloReply>> m_response_message_queue{};
};

#endif  // GRPC_SESSION_H