/**
 * Copyright (c) 2019-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    std::for_each(opts.framework_paths.begin(), opts.framework_paths.end(),
                  [&](auto s) { (*repl)->AddFrameworkSearchPath(s); });
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
