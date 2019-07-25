#include <iostream>
#include <string>

#include "CommandLineOptions.h"
#include "REPL.h"

void SetupOptions(int argc, char **argv)
{
    CommandLineOptions opts = ParseCommandLineOptions(argc, argv);
    SetLoggingOptions(opts.logging_opts);
}

int main(int argc, char **argv)
{
    SetupOptions(argc, argv);

    std::string line;
    REPL repl;
    do
    {
        std::getline(std::cin, line);
    } while(repl.ExecuteSwift(line));
    return 0;
}
