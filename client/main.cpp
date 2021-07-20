#include <iostream>
#include <cstdlib>

#include "log_helper.h"

using namespace std;

extern int g_system_log_level;

void SignalHandler(int signal)
{
	LOG(LOG_LEVEL::DBG, "Received termination signal: %d", signal);
}

int main(int argc, char **argv)
{
	char *env_var;
	
	signal(SIGTERM, SignalHandler);
	signal(SIGKILL, SignalHandler);

	try
	{
		int a = 1;
		int b = 2;

		cout << "Client: " << (a+b) << endl;

		LOG(LOG_LEVEL::INF, "Client: end.");
	}
	catch (std::exception &e)
	{
		LOG(LOG_LEVEL::ERR, "Exception: %s", e.what());
		return 1;
	}

    return 0;
}