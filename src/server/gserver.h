#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include <grpcpp/server.h>
#include "grpctype.h"

#include "greeter.grpc.pb.h"
#include "gsession.h"

class GServer
{
public:
	GServer(const GServer &) = delete;
	GServer(GServer &&) = delete;
	GServer &operator=(const GServer &) = delete;
	GServer &operator=(GServer &&) = delete;

	static GServer &GetInstance()
	{
		static GServer kGlobalInstance; // thread safe only in C++11
		return kGlobalInstance;
	}

	bool Initialize(const std::string &server);
	void Run();
	void Stop();

	std::shared_ptr<GSession> AddSession();
	void RemoveSession(uint64_t session_id);
	std::shared_ptr<GSession> GetSession(uint64_t session_id);

private:
	GServer()= default;
	virtual ~GServer() = default;

public:
	std::atomic_bool m_running{false};
	std::atomic_uint64_t m_next_session_id{1};

	std::unique_ptr<::grpc::Server> m_grpc_server;
	Greeter::AsyncService m_greeter_async_service;

	std::unique_ptr<::grpc::ServerCompletionQueue> m_completion_queue_call{}; // https://grpc.github.io/grpc/cpp/classgrpc_1_1_completion_queue.html
	std::unique_ptr<::grpc::ServerCompletionQueue> m_completion_queue_notification{}; // for notifications by server, health checker
	
	std::mutex m_mutex_sessions{};  // std::shared_mutex ?
	std::unordered_map<uint64_t, std::shared_ptr<GSession>> m_sessions{};
};

#endif // GRPC_SERVER_H