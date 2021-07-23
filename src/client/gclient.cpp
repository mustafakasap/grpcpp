#include "gclient.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using namespace client_server::grpc::v1;

bool GClient::Initialize(const std::string &server)
{
    m_stub = CSService01::NewStub(grpc::CreateChannel(server, grpc::InsecureChannelCredentials()));
    return true;
}

void GClient::Run()
{
    LOG(LOG_LEVEL::DBG, "Waiting server availability.");
    m_context.set_wait_for_ready(true);
    m_client_reader_writer_stream = m_stub->CSServiceMethod01(&m_context);

    m_running = true;

    LOG(LOG_LEVEL::DBG, "Start request/response threads.");
    thread_handle_server_response = std::thread(&GClient::handle_server_response, this);
    thread_make_server_request = std::thread(&GClient::make_server_request, this);

    if (thread_handle_server_response.joinable())
    {
        thread_handle_server_response.join();
    }
    if (thread_make_server_request.joinable())
    {
        thread_make_server_request.join();
    }
}

void GClient::handle_server_response()
{
    LOG(LOG_LEVEL::DBG, "start thread handle_server_response.");

    CSService01Method01MessageOut01 message_from_server;
    while (m_client_reader_writer_stream->Read(&message_from_server))
    {
        LOG(LOG_LEVEL::DBG, "received message from server: %s", message_from_server.ShortDebugString().c_str());
    }

    LOG(LOG_LEVEL::DBG, "stop thread handle_server_response.");
}

void GClient::make_server_request()
{
    LOG(LOG_LEVEL::DBG, "start thread make_server_request.");

    while (m_running)
    {
        try
        {
            CSService01Method01MessageIn01 message_to_server;

            LOG(LOG_LEVEL::DBG, "sending message to server: m_seq_num=%d", m_seq_num);
            message_to_server.set_csservice01method01messagein01param01(std::to_string(m_seq_num++));
            if (!m_client_reader_writer_stream->Write(message_to_server))
            {
                LOG(LOG_LEVEL::ERR, "Cant write to server.");
                break;
            }
        }
        catch (std::exception &e)
        {
            LOG(LOG_LEVEL::ERR, "Exception: %s", e.what());
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    LOG(LOG_LEVEL::DBG, "stop thread make_server_request.");
}


void GClient::Stop()
{
    if (!m_running)
    {
        return;
    }
    m_running = false;

    std::cout << "m_context->TryCancel() begin" << std::endl;
    m_context.TryCancel();

    std::cout << "m_context->TryCancel() end" << std::endl;
}