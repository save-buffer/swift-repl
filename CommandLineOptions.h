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
};

CommandLineOptions ParseCommandLineOptions(int argc, char **argv);

#endif
