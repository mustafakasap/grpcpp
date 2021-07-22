// https://gist.github.com/ppLorins/6e4cc625c2c5b8fd16ced3172b1ada09

#include <csignal>
#include <iostream>

#include <memory>
#include <string>
#include <thread>
#include <grpc++/grpc++.h>
#include "client_server_bidi_async_stream.grpc.pb.h"

using namespace std;
using namespace client_server::grpc::v1;

using grpc::Server;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;

enum class Type
{
	READ = 1,
	WRITE = 2,
	CONNECT = 3,
	DONE = 4,
	FINISH = 5
};

class AsyncBidiBiDiStreamServer
{
public:
	AsyncBidiBiDiStreamServer()
	{
		// In general avoid setting up the server in the main thread (specifically,
		// in a constructor-like function such as this). We ignore this in the
		// context of an example.
		std::string server_address("0.0.0.0:44444");

		ServerBuilder builder;
		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service_);
		cq_ = builder.AddCompletionQueue();
		server_ = builder.BuildAndStart();

		// This initiates a single stream for a single client. To allow multiple
		// clients in different threads to connect, simply 'request' from the
		// different threads. Each stream is independent but can use the same
		// completion queue/context objects.
		stream_.reset(
			new ServerAsyncReaderWriter<CSService01Method01MessageOut01, CSService01Method01MessageIn01>(&context_));

		service_.RequestCSServiceMethod01(&context_,
										  stream_.get(),
										  cq_.get(),
										  cq_.get(),
										  reinterpret_cast<void *>(Type::CONNECT));

		// This is important as the server should know when the client is done.
		context_.AsyncNotifyWhenDone(reinterpret_cast<void *>(Type::DONE));

		grpc_thread_.reset(new std::thread(
			(std::bind(&AsyncBidiBiDiStreamServer::GrpcThread, this))));
		std::cout << "Server listening on " << server_address << std::endl;
	}

	void SetResponse(const std::string &response)
	{
		if (response == "quit" && IsRunning())
		{
			stream_->Finish(grpc::Status::CANCELLED,
							reinterpret_cast<void *>(Type::FINISH));
		}
		response_str_ = response;
	}

	~AsyncBidiBiDiStreamServer()
	{
		std::cout << "Shutting down server...." << std::endl;
		server_->Shutdown();
		// Always shutdown the completion queue after the server.
		cq_->Shutdown();
		grpc_thread_->join();
	}

	bool IsRunning() const { return is_running_; }

private:
	void AsyncWaitForHelloRequest()
	{
		if (IsRunning())
		{
			// In the case of the server, we wait for a READ first and then write a
			// response. A server cannot initiate a connection so the server has to
			// wait for the client to send a message in order for it to respond back.
			stream_->Read(&request_, reinterpret_cast<void *>(Type::READ));
		}
	}

	void AsyncHelloSendResponse()
	{
		std::cout << " ** Handling request: " << request_.csservice01method01messagein01param01() << std::endl;
		CSService01Method01MessageOut01 response;
		std::cout << " ** Sending response: " << response_str_ << std::endl;
		response.set_csservice01method01messageout01param01(response_str_);
		stream_->Write(response, reinterpret_cast<void *>(Type::WRITE));
	}

	void GrpcThread()
	{
		while (true)
		{
			void *got_tag = nullptr;
			bool ok = false;
			if (!cq_->Next(&got_tag, &ok))
			{
				std::cerr << "Server stream closed. Quitting" << std::endl;
				break;
			}

			//assert(ok);

			if (ok)
			{
				std::cout << std::endl
						  << "**** Processing completion queue tag " << got_tag
						  << std::endl;
				switch (static_cast<Type>(reinterpret_cast<size_t>(got_tag)))
				{
				case Type::READ:
					std::cout << "Read a new message." << std::endl;
					AsyncHelloSendResponse();
					break;
				case Type::WRITE:
					std::cout << "Sending message (async)." << std::endl;
					AsyncWaitForHelloRequest();
					break;
				case Type::CONNECT:
					std::cout << "Client connected." << std::endl;
					AsyncWaitForHelloRequest();
					break;
				case Type::DONE:
					std::cout << "Server disconnecting." << std::endl;
					is_running_ = false;
					break;
				case Type::FINISH:
					std::cout << "Server quitting." << std::endl;
					break;
				default:
					std::cerr << "Unexpected tag " << got_tag << std::endl;
					assert(false);
				}
			}
		}
	}

private:
	CSService01Method01MessageIn01 request_;
	std::string response_str_ = "Default server response";
	ServerContext context_;
	CSService01::AsyncService service_;
	std::unique_ptr<ServerCompletionQueue> cq_;
	std::unique_ptr<Server> server_;
	std::unique_ptr<ServerAsyncReaderWriter<CSService01Method01MessageOut01, CSService01Method01MessageIn01>> stream_;
	std::unique_ptr<std::thread> grpc_thread_;
	bool is_running_ = true;
};

void SignalHandler(int signal)
{
	cout << "Received termination signal: " << signal << endl;
}

int main(int argc, char **argv)
{
	cout << "Starting Server..." << endl;

	signal(SIGTERM, SignalHandler);
	signal(SIGINT, SignalHandler);
	signal(SIGKILL, SignalHandler);

	try
	{
		AsyncBidiBiDiStreamServer server;

		int64_t counter = -1;

		std::string response;
		while (server.IsRunning())
		{
			response = to_string(counter--);
			cout << "Server sending: " << response << endl;
			server.SetResponse(response);

			sleep(1);
		}
	}
	catch (std::exception &e)
	{
		cout << "Exception: " << e.what() << endl;
		return -1;
	}

	return 0;
}