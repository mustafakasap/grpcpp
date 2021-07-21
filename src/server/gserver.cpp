#include "gserver.h"
#include <grpcpp/grpcpp.h>
#include <thread>

#include "grpctype.h"
#include "log_helper.h"


bool GServer::Initialize(const std::string &server_address)
{
	LOG(LOG_LEVEL::DBG, "Initializing server...");
	::grpc::ServerBuilder builder;

	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

	builder.RegisterService(&m_greeter_async_service);

	m_completion_queue_call = builder.AddCompletionQueue();
	m_completion_queue_notification = builder.AddCompletionQueue();

	m_grpc_server = builder.BuildAndStart();

	return true;
}

void GServer::Run()
{
	m_running = true;

    LOG(LOG_LEVEL::DBG, "Run> Server start run.");

	// First thread: Completion Queue - Call
	std::thread thread_completion_queue_call([this]
	{
		LOG(LOG_LEVEL::DBG, "Start thread_completion_queue_call.");

		// Add first idle session waiting for connection
		AddSession();

		uint64_t session_id; // <session_id, session> map id for distinct connection sessions
		bool ok;

		// Read from the queue, blocking until an event is available or the queue is shutting down.
		// "read done / write done / close(already connected)"
		while (m_completion_queue_call->Next(reinterpret_cast<void **>(&session_id), &ok))
		{
			auto event = static_cast<GrpcEvent>(session_id & GRPC_EVENT_MASK);
			session_id = session_id >> GRPC_EVENT_BIT_LENGTH;
			LOG(LOG_LEVEL::DBG, "completion queue call, session id: %d, event: %d", session_id, event);

			if (event == GRPC_EVENT_FINISHED)
			{
				RemoveSession(session_id);
				continue;
			}

			auto session = GetSession(session_id);
			if (session == nullptr)
			{
				LOG(LOG_LEVEL::DBG, "completion queue call: session id: %d, session doesnt exist.", session_id);
				continue;
			}
			if (!ok)
			{
				LOG(LOG_LEVEL::DBG, "completion queue call: session id: %d, rpc call closed.", session_id);
				RemoveSession(session_id);
				continue;
			}

			std::lock_guard<std::mutex> local_lock_guard{session->m_mutex};
			session->Process(event);
		}
		
		LOG(LOG_LEVEL::DBG, "completion queue call: Loop end, exiting...");
	});

	// Second thread: Completion Queue - Notification
	std::thread thread_completion_queue_notification([this]
	{
		LOG(LOG_LEVEL::DBG, "Start thread_completion_queue_notification.");

		uint64_t session_id; // <session_id, session> map id for distinct connection sessions
		bool ok;

		// A specific type of completion queue used by the processing of notifications by servers. 
		// "new connection / close(waiting for connect)"
		while (m_completion_queue_notification->Next(reinterpret_cast<void **>(&session_id), &ok))
		{
			auto event = static_cast<GrpcEvent>(session_id & GRPC_EVENT_MASK);
			session_id = session_id >> GRPC_EVENT_BIT_LENGTH;
			LOG(LOG_LEVEL::DBG, "completion queue notification call, session id: %d, event: %d", session_id, event);

			if (event == GRPC_EVENT_FINISHED)
			{
				RemoveSession(session_id);
				continue;
			}
			auto session = GetSession(session_id);
			if (session == nullptr)
			{
				LOG(LOG_LEVEL::DBG, "completion queue notification call: session id: %d, session doesnt exist.", session_id);
				continue;
			}
			if (!ok)
			{
				LOG(LOG_LEVEL::DBG, "completion queue notification call: session id: %d, rpc call closed.", session_id);
				RemoveSession(session_id);
				continue;
			}

			std::lock_guard<std::mutex> local_lock_guard{session->m_mutex};
			session->Process(event);
		}

		LOG(LOG_LEVEL::DBG, "completion queue notification call: Loop end, exiting...");
	});

	// Third thread:Responder
	std::thread thread_respond([this]
	{
		LOG(LOG_LEVEL::DBG, "Start thread_respond.");

		while (m_running)
		{
			// if too short, it will lock continiously...
			std::this_thread::sleep_for(std::chrono::milliseconds(10));

			std::lock_guard<std::mutex> local_lock_guard{m_mutex_sessions};
			for (const auto &it : m_sessions)
			{
				// std::lock_guard<std::mutex> inner_local_lock_guard{it.second->m_mutex};
				it.second->Reply();
			}
		}
	});

	// clean-up, wait threads to complete...
	if (thread_completion_queue_call.joinable())
	{
		thread_completion_queue_call.join();
	}
	if (thread_completion_queue_notification.joinable())
	{
		thread_completion_queue_notification.join();
	}
	if (thread_respond.joinable())
	{
		thread_respond.join();
	}
	
	LOG(LOG_LEVEL::DBG, "GRPC server RUN exiting...");
}

void GServer::Stop()
{
	if (!m_running) return;
	m_running = false;

	LOG(LOG_LEVEL::DBG, "Starting to TryCancel() all sessions...");

	{	// scoped lock
		std::lock_guard<std::mutex> local_lock_guard{m_mutex_sessions};
		for (const auto &it : m_sessions)
		{
			std::lock_guard<std::mutex> local_lock_guard_inner{it.second->m_mutex};
			if (it.second->m_status != GrpcSessionStatus::WAIT_CONNECT)
			{
				it.second->m_server_context.TryCancel();
			}
		}
	}

	LOG(LOG_LEVEL::DBG, "Shutting down grpc server...");
	m_grpc_server->Shutdown();

	LOG(LOG_LEVEL::DBG, "Shutting down completion queue(call)...");
	m_completion_queue_call->Shutdown();

	LOG(LOG_LEVEL::DBG, "Shutting down completion queue(notification)...");
	m_completion_queue_notification->Shutdown();

	LOG(LOG_LEVEL::DBG, "Shutting down grpc server complete.");
}

std::shared_ptr<GSession> GServer::AddSession()
{
	auto new_session_id = m_next_session_id++;
	auto new_session = std::make_shared<GSession>(new_session_id);
	if (!new_session->Initialize())
	{
		LOG(LOG_LEVEL::DBG, "Cannot initialize the new session.");
		return nullptr;
	}

	std::lock_guard<std::mutex> local_lock_guard{m_mutex_sessions};
	m_sessions[new_session_id] = new_session;

	LOG(LOG_LEVEL::DBG, "AddSession> session id: %d, new session is active and waiting for connection.", new_session_id);
	return new_session;
}

void GServer::RemoveSession(uint64_t session_id)
{
	std::lock_guard<std::mutex> local_lock_guard{m_mutex_sessions};
	m_sessions.erase(session_id);
}

std::shared_ptr<GSession> GServer::GetSession(uint64_t session_id)
{
	std::lock_guard<std::mutex> local_lock_guard{m_mutex_sessions};
	auto it = m_sessions.find(session_id);
	if (it == m_sessions.end())
	{
		return nullptr;
	}
	return it->second;
}