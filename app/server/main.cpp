#include <csignal>
#include <iostream>

#include "log_helper.h"

using namespace std;


void SignalHandler(int signal)
{
	LOG(LOG_LEVEL::DBG, "Received termination signal: %d", signal);
}

int main(int argc, char **argv)
{
	signal(SIGTERM, SignalHandler);
	signal(SIGINT, SignalHandler);

	try
	{
		int a = 1;
		int b = 2;

		cout << "Server: " << (a+b) << endl;

		LOG(LOG_LEVEL::INF, "Server: end.");
	}
	catch (std::exception &e)
	{
		LOG(LOG_LEVEL::ERR, "Exception: %s", e.what());
		return -1;
	}

	return 0;
}