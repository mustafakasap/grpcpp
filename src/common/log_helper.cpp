#include <iostream>
#include <string>
#include <chrono>
#include <stdarg.h>

#include "log_helper.h"

using namespace std;

int g_system_log_level = 2;

std::string LevelToString(LOG_LEVEL level)
{
    const string strLevel[]  {"ERR", "INF", "DBG"};
    return strLevel[(int)level];
}

string GetSystemTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm* ptm = localtime(&tt);
	char date[60] = { 0 };
	sprintf(date, "%02d:%02d:%02d ",
		(int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
	return std::string(date);
}

void LOG(LOG_LEVEL level, const char *fmt, ...)
{
    if (static_cast<int>(level) > g_system_log_level)
        return;

    #ifdef SERVER
        string target = "SRV";
    #else
        string target = "CLI";
    #endif
    string strlevel = LevelToString(level);
    std::string buf1 = GetSystemTime() + strlevel + ": ";

    cout << target << " " << buf1;

    va_list val;
    va_start(val, fmt);
    vprintf(fmt, val);
    va_end(val);

    cout << endl;
    
    return ;
}