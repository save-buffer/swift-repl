#include "CommandLineOptions.h"

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

void ParseSingleCommandLineOption(std::string arg, CommandLineOptions &opts)
{
    using OptionParseFn = std::add_pointer<void(std::string, std::string, CommandLineOptions &)>::type;
    auto delimeter_pos = arg.find("=");
    std::string opt = arg.substr(0, delimeter_pos);
    std::string val = arg.substr(delimeter_pos + 1, arg.size() - opt.size() - 1);
    llvm::StringSwitch<OptionParseFn>(opt)
        .Case("--logging", SetLoggingAreaOption)
        .Case("--logging_priority", SetLoggingPriorityOption)
        .Case("--playground", SetPlaygroundOption)
        .Default(HandleUnknownOption)
        (opt, val, opts);
}

//TODO(sasha): Make these show no logging by default in the future.
void SetupDefaultsIfUninitialized(CommandLineOptions &opts)
{
    if(opts.logging_opts.log_areas == LoggingArea::Unknown)
        opts.logging_opts.log_areas = LoggingArea::All;

    if(opts.logging_opts.min_priority == LoggingPriority::Unknown)
        opts.logging_opts.min_priority = LoggingPriority::Info;
}

CommandLineOptions ParseCommandLineOptions(int argc, char **argv)
{
    CommandLineOptions result;
    for(int i = 1; i < argc; i++)
    {
        ParseSingleCommandLineOption(argv[i], result);
    }
    SetupDefaultsIfUninitialized(result);
    return result;
}
