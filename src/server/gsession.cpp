#include "gsession.h"
#include "gserver.h"

#include "log_helper.h"
#include <fcntl.h>			// unix os level file open/read/write
#include <unistd.h>			// file handle close
#include <math.h> 
#include <functional>
#include <algorithm>
#include <fstream>
#include <string>
#include <iostream>
#include <iomanip>

using namespace client_server::grpc::v1;

bool GSession::Initialize()
{
	// https://grpc.github.io/grpc/cpp/classgrpc_1_1_async_generic_service.html
	GServer::GetInstance().m_greeter_async_service.RequestSayHello(
		&m_server_context,
		&m_subscribe_stream,
		GServer::GetInstance().m_completion_queue_call.get(),
		GServer::GetInstance().m_completion_queue_notification.get(),
		reinterpret_cast<void *>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_CONNECTED)
	);

	return true;
}

void GSession::Process(GrpcEvent event) {
	LOG(LOG_LEVEL::DBG, "Session Process, session id: %d, event: %d", m_session_id, event);
	switch (event)
	{
		case GRPC_EVENT_CONNECTED:
			// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
			m_subscribe_stream.Read(
				&m_request,
				reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_READ_DONE));
			
			m_status = GrpcSessionStatus::READY_TO_WRITE;
			GServer::GetInstance().AddSession();	// new session to wait for  the next connection...
			return;

		case GRPC_EVENT_READ_DONE:
			LOG(LOG_LEVEL::DBG, "GRPC_EVENT_READ_DONE, session id: %d, event: %d", m_session_id, event);
			LOG(LOG_LEVEL::INF, "Request.Name: %s", m_request.name().c_str());

			// LOG(LOG_LEVEL::DBG, "Session Process read done, session id: %d, event: %d \n\t request: %s", m_session_id, event, m_request.DebugString().c_str());
			LOG(LOG_LEVEL::DBG, "Session Process read done, session id: %d, event: %d", m_session_id, event);

			// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
			m_subscribe_stream.Read(
				&m_request,
				reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_READ_DONE));

			Reply();
			return;

		case GRPC_EVENT_WRITE_DONE:
			if (!m_response_message_queue.empty())
			{
				m_status = GrpcSessionStatus::WAIT_WRITE_DONE;
				
				// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
				m_subscribe_stream.Write(
					*m_response_message_queue.front(),
					reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_WRITE_DONE));

				m_response_message_queue.pop_front();
			} else {
				m_status = GrpcSessionStatus::READY_TO_WRITE;
			}
			return;

		default:
			LOG(LOG_LEVEL::ERR, "Unhandled session event, session id: %d, event: %d", m_session_id, event);
			return;
	}
}

void GSession::Reply()
{
	if (m_status != GrpcSessionStatus::READY_TO_WRITE && m_status != GrpcSessionStatus::WAIT_WRITE_DONE)
		return;

	LOG(LOG_LEVEL::DBG, "Session Reply, session id: %d, status: %d", m_session_id, m_status);

	// Here we are sending randomly generated replies... 
	// User may fill the m_response_message_queue within Client Read stage with responses to client regarding client's requests.
	auto new_message = std::make_shared<HelloReply>();
	new_message->set_message(std::to_string(m_seq_num--));

	if (m_status == GrpcSessionStatus::READY_TO_WRITE)
	{
		m_status = GrpcSessionStatus::WAIT_WRITE_DONE;

		// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
		m_subscribe_stream.Write(
			*new_message,
			reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_WRITE_DONE));

	} else {
		m_response_message_queue.emplace_back(new_message);
	}
}

void GSession::Finish()
{
	if (m_status == GrpcSessionStatus::WAIT_CONNECT) { return; }
	m_subscribe_stream.Finish(
		::grpc::Status::CANCELLED,
		reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_FINISHED));
}
