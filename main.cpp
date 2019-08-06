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

    llvm::Expected<std::unique_ptr<REPL>> repl = REPL::Create();
    if(!repl)
    {
        std::string err_str;
        llvm::raw_string_ostream stream(err_str);
        stream << repl.takeError();
        stream.flush();
        SetCurrentLoggingArea(LoggingArea::All);
        Log(err_str, LoggingPriority::Error);
        return 1;
    }

    std::string line;
    do
    {
        line = (*repl)->GetLine();
    } while((*repl)->ExecuteSwift(line));
    return 0;
}
