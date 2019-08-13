#ifndef COMMAND_LINE_OPTIONS_H
#define COMMAND_LINE_OPTIONS_H

#include "Logging.h"

#include <string>
#include <vector>

struct CommandLineOptions
{
    LoggingOptions logging_opts;
    bool is_playground;
    std::vector<std::string> include_paths;
    std::vector<std::string> link_paths;
};

CommandLineOptions ParseCommandLineOptions(int argc, char **argv);

#endif
