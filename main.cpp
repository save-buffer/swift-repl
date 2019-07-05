#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    std::string line;
    for(;;)
    {
        std::getline(std::cin, line);
        if(line == "e" || line == "exit")
        {
            return 0;
        }
    }
    return 0;
}
