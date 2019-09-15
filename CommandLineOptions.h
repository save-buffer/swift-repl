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

#ifndef COMMAND_LINE_OPTIONS_H
#define COMMAND_LINE_OPTIONS_H

#include "Logging.h"

#include <string>
#include <vector>

struct CommandLineOptions
{
    LoggingOptions logging_opts;
    bool is_playground;
    bool print_to_console;
    std::string default_module_cache_path;
    std::vector<std::string> include_paths;
    std::vector<std::string> link_paths;
    std::vector<std::string> framework_paths;
};

CommandLineOptions ParseCommandLineOptions(int argc, char **argv);

#endif
