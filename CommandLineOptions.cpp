#include "CommandLineOptions.h"
#include "Config.h"
#include "Strings.h"

#include <iostream>
#include <string>
#include <type_traits>

#include <llvm/ADT/StringSwitch.h>

void HandleUnknownOption(std::string opt, std::string val, CommandLineOptions &opts)
{
    // We format this manually to look like the Logging system because Logging system isn't initialized yet. 
    std::cout << "[Warning] Ignoring unrecognized parameter \"" << opt << "\" with value " << val << "\n";
}

void SetLoggingAreaOption(std::string opt, std::string val, CommandLineOptions &opts)
{
    auto area = llvm::StringSwitch<LoggingArea>(val)
        .Case(     "ast", LoggingArea::AST)
        .Case(     "sil", LoggingArea::SIL)
        .Case(      "ir", LoggingArea:: IR)
        .Case(     "jit", LoggingArea::JIT)
        .Case("importer", LoggingArea::Importer)
        .Case(     "all", LoggingArea::All)
        .Default(LoggingArea::Unknown);

    if(area == LoggingArea::Unknown)
        std::cout << "[Warning] Ignoring unrecognized logging area \"" << val << "\n";
    else
        opts.logging_opts.log_areas |= area;
}

void SetLoggingPriorityOption(std::string opt, std::string val, CommandLineOptions &opts)
{
    auto priority = llvm::StringSwitch<LoggingPriority>(val)
        .Case("info", LoggingPriority::Info)
        .Case("warning", LoggingPriority::Warning)
        .Case("error", LoggingPriority::Error)
        .Case("none", LoggingPriority::None)
        .Default(LoggingPriority::Unknown);
    
    if(priority == LoggingPriority::Unknown)
        std::cout << "[Warning] Ignoring unrecognized logging priority \"" << val << "\n";
    else
        opts.logging_opts.min_priority = priority;
}

void SetPlaygroundOption(std::string opt, std::string val, CommandLineOptions &opts)
{
    int is_playground = llvm::StringSwitch<int>(val)
        .Case("true", 1)
        .Case("false", 0)
        .Default(-1);
    if(is_playground == -1)
        std::cout << "[Warning] is_playground is neither \"true\" nor \"false\". Defaulting to \"false\"\n";
    opts.is_playground = is_playground == 1;
}

void SetModuleCachePathOption(std::string opt, std::string val, CommandLineOptions &opts)
{
    opts.default_module_cache_path = val;
}

void SetPrintToConsoleOption(std::string opt, std::string val, CommandLineOptions &opts)
{
    int print_to_console = llvm::StringSwitch<int>(val)
        .Case("true", 1)
        .Case("false", 0)
        .Default(-1);
    if(print_to_console == -1)
        std::cout << "[Warning] print_to_console is neither \"true\" nor \"false\". Defaulting to \"true\"\n";
    opts.print_to_console = print_to_console != 0;
}

void HandleOptionWithoutEquals(std::string arg, CommandLineOptions &opts)
{
    // NOTE(sasha): The 2 comes from the length of "-i" or "-l"
    if(StartsWith(arg, "-I"))
        opts.include_paths.push_back(arg.substr(2));
    else if(StartsWith(arg, "-L"))
        opts.link_paths.push_back(arg.substr(2));
    else if(StartsWith(arg, "-F"))
        opts.framework_paths.push_back(arg.substr(2));
    else if(arg == "--print_to_console")
        opts.print_to_console = true;
    else
        std::cout << "[Warning] Ignoring unrecognized parameter \"" << arg << "\"\n";
}

void ParseSingleCommandLineOption(std::string arg, CommandLineOptions &opts)
{
    using OptionParseFn = std::add_pointer<void(std::string, std::string, CommandLineOptions &)>::type;
    auto delimeter_pos = arg.find("=");
    if(delimeter_pos == std::string::npos)
        return HandleOptionWithoutEquals(arg, opts);

    ToLowerCase(arg);
    std::string opt = arg.substr(0, delimeter_pos);
    std::string val = arg.substr(delimeter_pos + 1, arg.size() - opt.size() - 1);
    llvm::StringSwitch<OptionParseFn>(opt)
        .Case("--logging", SetLoggingAreaOption)
        .Case("--logging_priority", SetLoggingPriorityOption)
        .Case("--playground", SetPlaygroundOption)
        .Case("--module_cache_path", SetModuleCachePathOption)
        .Case("--print_to_console", SetPrintToConsoleOption)
        .Default(HandleUnknownOption)
        (opt, val, opts);
}

CommandLineOptions DefaultCommandLineOptions()
{
    CommandLineOptions opts;
    opts.logging_opts.log_areas = LoggingArea::All;
    opts.logging_opts.min_priority = LoggingPriority::None;
    opts.default_module_cache_path = DEFAULT_MODULE_CACHE_PATH;
    opts.is_playground = false;
    opts.print_to_console = false;
    return opts;
}

//TODO(sasha): Make this more robust to things like -I <path> (with a space)
CommandLineOptions ParseCommandLineOptions(int argc, char **argv)
{
    CommandLineOptions result = DefaultCommandLineOptions();
    for(int i = 1; i < argc; i++)
    {
        std::string sanitized_option = argv[i];
        Trim(sanitized_option);
        ParseSingleCommandLineOption(sanitized_option, result);
    }
    return result;
}
