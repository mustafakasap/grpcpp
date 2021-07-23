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
#include "client_server_bidi_async_stream.grpc.pb.h"

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
    int64_t m_seq_num = -1;

    const uint64_t m_session_id{0};
    ::grpc::ServerAsyncReaderWriter<CSService01Method01MessageOut01, CSService01Method01MessageIn01> m_server_reader_writer_stream{&m_server_context};

    CSService01Method01MessageIn01 m_message_from_client{};
    std::deque<std::shared_ptr<CSService01Method01MessageOut01>> m_message_to_client_queue{};
};

#endif  // GRPC_SESSION_H