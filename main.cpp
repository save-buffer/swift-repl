#include <iostream>
#include <string>

#include "CommandLineOptions.h"
#include "REPL.h"

std::unique_ptr<REPL> SetupREPLWithOptions(int argc, char **argv)
{
    CommandLineOptions opts = ParseCommandLineOptions(argc, argv);
    SetLoggingOptions(opts.logging_opts);

    llvm::Expected<std::unique_ptr<REPL>> repl = REPL::Create(
        opts.is_playground, opts.default_module_cache_path);
    if(!repl)
    {
        std::string err_str;
        llvm::raw_string_ostream stream(err_str);
        stream << repl.takeError();
        stream.flush();
        SetCurrentLoggingArea(LoggingArea::All);
        Log(err_str, LoggingPriority::Error);
        return nullptr;
    }

    std::for_each(opts.include_paths.begin(), opts.include_paths.end(),
                  [&](auto s) { (*repl)->AddModuleSearchPath(s); });
    std::for_each(opts.link_paths.begin(), opts.link_paths.end(),
                  [&](auto s) { (*repl)->AddLoadSearchPath(s); });
    return std::move(*repl);
}

int main(int argc, char **argv)
{
    std::unique_ptr<REPL> repl = SetupREPLWithOptions(argc, argv);
    if(!repl)
        return 1;

    std::string line;
    do
    {
        line = repl->GetLine();
    } while(repl->ExecuteSwift(line));
    return 0;
}
