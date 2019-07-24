#ifndef LOGGING_H
#define LOGGING_H

#include <cstdint>
#include <string>

// To use this logging system:
//  - Set it up by settings bits corresponding to the
//    areas you want to log and set the maximal priority
//    of the logs
//  - When entering a new segment of code, SetCurrentLoggingArea()
//    to the area corresponding to that segment
//  - Log with Log()

enum class LoggingArea : uint64_t
{
    Unknown = 0, 
    AST = (1 << 0),
    SIL = (1 << 1),
    IR  = (1 << 2),
    JIT = (1 << 3),
    Importer = (1 << 4),
    All = ~0,
};

enum class LoggingPriority : uint64_t
{
    Info,
    Warning,
    Error,
    None,
    Unknown,
};

LoggingArea operator &(LoggingArea l, LoggingArea r);
LoggingArea operator |(LoggingArea l, LoggingArea r);
LoggingArea &operator &=(LoggingArea &l, LoggingArea r);
LoggingArea &operator |=(LoggingArea &l, LoggingArea r);

struct LoggingOptions
{
    LoggingArea log_areas = LoggingArea::Unknown;
    LoggingPriority min_priority = LoggingPriority::Unknown;
};

void Log(std::string message, LoggingPriority priority = LoggingPriority::Info);
bool ShouldLog(LoggingPriority priority = LoggingPriority::Error);
void SetCurrentLoggingArea(LoggingArea area);
void SetLoggingOptions(LoggingOptions opts);

#endif
