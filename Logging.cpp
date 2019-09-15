/**
 * Copyright (c) 2019-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

const char *PriorityStrings[] = { "[INFO] ", "[WARNING] ", "[ERROR] ", "", "" };

void SetLoggingOptions(LoggingOptions opts)
{
    g_log_opts = opts;
}

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
