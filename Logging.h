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

void SetLoggingOptions(LoggingOptions opts);
void Log(std::string message, LoggingPriority priority = LoggingPriority::Info);
bool ShouldLog(LoggingPriority priority = LoggingPriority::Error);
void SetCurrentLoggingArea(LoggingArea area);

#endif
