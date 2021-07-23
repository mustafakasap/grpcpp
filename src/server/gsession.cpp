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
	GServer::GetInstance().m_cs_async_service.RequestCSServiceMethod01(
		&m_server_context,
		&m_server_reader_writer_stream,
		GServer::GetInstance().m_completion_queue.get(),
		GServer::GetInstance().m_completion_queue.get(),
		reinterpret_cast<void *>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_CONNECTED)
	);

	return true;
}

void GSession::Process(GrpcEvent event) 
{
	switch (event)
	{
		case GRPC_EVENT_CONNECTED:
			// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
			m_server_reader_writer_stream.Read(
				&m_message_from_client,
				reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_READ_DONE));
			
			m_status = GrpcSessionStatus::READY_TO_WRITE;
			GServer::GetInstance().AddSession();	// new session to wait for  the next connection...
			return;

		case GRPC_EVENT_READ_DONE:
			// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
			m_server_reader_writer_stream.Read(
				&m_message_from_client,
				reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_READ_DONE));

			LOG(LOG_LEVEL::INF, 
					"Received message: session id: %d, event: %d, received: %s", 
						m_session_id, 
						event, 
						m_message_from_client.ShortDebugString().c_str());
			return;

		case GRPC_EVENT_WRITE_DONE:
			if (!m_message_to_client_queue.empty())
			{
				m_status = GrpcSessionStatus::WAIT_WRITE_DONE;
				
				LOG(LOG_LEVEL::INF, "Sending message 2: %s", (*m_message_to_client_queue.front()).ShortDebugString().c_str());
				// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
				m_server_reader_writer_stream.Write(
					*m_message_to_client_queue.front(),
					reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_WRITE_DONE));

				m_message_to_client_queue.pop_front();
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
	auto new_message = std::make_shared<CSService01Method01MessageOut01>();
	new_message->set_csservice01method01messageout01param01(std::to_string(m_seq_num--));

	if (m_status == GrpcSessionStatus::READY_TO_WRITE)
	{
		m_status = GrpcSessionStatus::WAIT_WRITE_DONE;

		LOG(LOG_LEVEL::INF, "Sending message 1: %s", (*new_message).ShortDebugString().c_str());

		// https://grpc.github.io/grpc/cpp/classgrpc_1_1_server_async_reader_writer.html
		m_server_reader_writer_stream.Write(
			*new_message,
			reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_WRITE_DONE));

	} else {
		m_message_to_client_queue.emplace_back(new_message);
	}
}

void GSession::Finish()
{
	if (m_status == GrpcSessionStatus::WAIT_CONNECT) { return; }
	m_server_reader_writer_stream.Finish(
		::grpc::Status::CANCELLED,
		reinterpret_cast<void*>(m_session_id << GRPC_EVENT_BIT_LENGTH | GRPC_EVENT_FINISHED));
}
