#include <csignal>
#include <iostream>

#include "log_helper.h"
#include "gclient.h"

using namespace std;

extern int g_system_log_level;

void SignalHandler(int signal)
{
	LOG(LOG_LEVEL::DBG, "Received termination signal: %d", signal);
	GClient::GetInstance().Stop();
}

int main(int argc, char **argv)
{
	signal(SIGTERM, SignalHandler);
	signal(SIGINT, SignalHandler);
	signal(SIGKILL, SignalHandler);

	try
	{
		std::string grpc_server_address = "localhost:44444";
		char *env_var = std::getenv("GRPC_SERVER_ADDRESS");
		if (env_var != NULL)
			grpc_server_address.assign(env_var);

		g_system_log_level = 2;
		env_var = std::getenv("LOGGING_LEVEL");
		if (env_var != NULL)
		{
			std::string temp(env_var);
			g_system_log_level = std::stoi(temp);
		}

		LOG(LOG_LEVEL::INF, "GRPC Client starting...");
		LOG(LOG_LEVEL::INF, "GRPC_SERVER_ADDRESS: %s", grpc_server_address.c_str());
		LOG(LOG_LEVEL::INF, "LOGGING_LEVEL: %d", g_system_log_level);

		if (!GClient::GetInstance().Initialize(grpc_server_address))
		{
            LOG(LOG_LEVEL::ERR, "Client initialization failed.");
			return -1;
		}

		GClient::GetInstance().Run();
	}
	catch (std::exception &e)
	{
		LOG(LOG_LEVEL::ERR, "Exception: %s", e.what());
		return -1;
	}

	return 0;
}