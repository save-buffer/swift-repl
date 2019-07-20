#include <iostream>
#include <string>

#include "REPL.h"

int main(int argc, char **argv)
{
    std::string line;
    REPL repl;
    do
    {
        std::getline(std::cin, line);
    } while(repl.ExecuteSwift(line));
    return 0;
}
