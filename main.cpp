#include <iostream>
#include <string>

#include "CommandLineOptions.h"
#include "REPL.h"

int main(int argc, char **argv)
{
    ParseCommandLineOptions(argc, argv);
    std::string line;
    REPL repl;
    do
    {
        std::getline(std::cin, line);
    } while(repl.ExecuteSwift(line));
    return 0;
}
