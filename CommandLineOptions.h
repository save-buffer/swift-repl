#ifndef COMMAND_LINE_OPTIONS_H
#define COMMAND_LINE_OPTIONS_H

#include "Logging.h"

struct CommandLineOptions
{
    LoggingOptions logging_opts;
};

CommandLineOptions ParseCommandLineOptions(int argc, char **argv);

#endif
