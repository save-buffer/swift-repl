#include <iostream>
#include <string>

struct REPL
{
    bool IsExitString(const std::string &line)
    {
        return line == "e" || line == "exit";
    }

    bool ExecuteSwift(std::string line)
    {
        if(IsExitString(line))
            return false;
        return true;
    }
};

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
