#include <csignal>
#include <iostream>

#include "gserver.h"
#include "log_helper.h"

using namespace std;

extern int g_system_log_level;

void SignalHandler(int signal)
{
	LOG(LOG_LEVEL::DBG, "Received termination signal: %d", signal);
	GServer::GetInstance().Stop();
}

int main(int argc, char **argv)
{
	signal(SIGTERM, SignalHandler);
	signal(SIGINT, SignalHandler);
	signal(SIGKILL, SignalHandler);

	try
	{
		g_system_log_level = 1;
		
		char *env_var = std::getenv("LOGGING_LEVEL");
		if (env_var != NULL)
		{
			std::string temp(env_var);
			g_system_log_level = std::stoi(temp);
		}

		// Should improve arg parsing...
		std::string grpc_server_address{"0.0.0.0:44444"};
		if (argc >= 2)
			grpc_server_address = argv[1];


		LOG(LOG_LEVEL::INF, "GRPC Server starting...");
		LOG(LOG_LEVEL::INF, "GRPC_SERVER_ADDRESS: %s", grpc_server_address.c_str());
		LOG(LOG_LEVEL::INF, "LOGGING_LEVEL: %d", g_system_log_level);


		if (!GServer::GetInstance().Initialize(grpc_server_address))
		{
			LOG(LOG_LEVEL::ERR, "Server initialization failed.");
			return -1;
		}

		GServer::GetInstance().Run();
	}
	catch (std::exception &e)
	{
		LOG(LOG_LEVEL::ERR, "Exception: %s", e.what());
		return -1;
	}

	return 0;
}