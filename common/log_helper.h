#ifndef LOG_H
#define LOG_H

/*
    NOTES:
        - Working fine because the callers are thread safe

    TODO:
        - Threadsafe
        - Convert to a class
        - Colorful output
*/

enum class LOG_LEVEL
{
    ERR = 0,
    INF = 1,
    DBG = 2
};

void LOG(LOG_LEVEL level, const char *fmt, ...);

#endif // LOG_H