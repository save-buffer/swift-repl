#include "Logging.h"

#include <iostream>

LoggingArea operator &(LoggingArea l, LoggingArea r)
{
    return static_cast<LoggingArea>(static_cast<uint64_t>(l) &
                                    static_cast<uint64_t>(r));
}

LoggingArea operator |(LoggingArea l, LoggingArea r)
{
    return static_cast<LoggingArea>(static_cast<uint64_t>(l) |
                                    static_cast<uint64_t>(r));
}

LoggingArea &operator &=(LoggingArea &l, LoggingArea r)
{
    l = l & r;
    return l;
}

LoggingArea &operator |=(LoggingArea &l, LoggingArea r)
{
    l = l | r;
    return l;
}

LoggingOptions g_log_opts;
LoggingArea g_curr_logging_area;

const char *PriorityStrings[] = { "", "[INFO] ", "[WARNING ]", "[ERROR] " };

void Log(std::string message, LoggingPriority priority)
{
    if(!ShouldLog(priority))
        return;
    
    std::cout << PriorityStrings[static_cast<uint64_t>(priority)] << message << "\n";
}

bool ShouldLog(LoggingPriority priority)
{
    return (g_log_opts.min_priority <= priority && priority < LoggingPriority::None) &&
                                    (static_cast<uint64_t>(g_log_opts.log_areas) & static_cast<uint64_t>(g_curr_logging_area));
}

void SetCurrentLoggingArea(LoggingArea area)
{
    g_curr_logging_area = area;
}

void SetLoggingOptions(LoggingOptions opts)
{
    g_log_opts = opts;
}
