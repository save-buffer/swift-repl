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

#include "LibraryLoading.h"
#include "Config.h"

#include <string>
#include <vector>

#include <iostream>

#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/FileSystem.h>

static std::vector<std::string> g_additional_paths;

void AddLibrarySearchPath(std::string path)
{
    llvm::SmallVector<char, 0> full_path;
    llvm::sys::fs::real_path(path, full_path, /* expand_tilde */ true);
    llvm::sys::fs::make_absolute(full_path);
    llvm::Twine path_twine(full_path);
    path = path_twine.str();

    g_additional_paths.push_back(path);
}

bool SearchForAndLoadLibraryPermanently(std::string name)
{
    // NOTE(sasha): For some strange reason, LLVM's LoadLibraryPermanently returns true
    //              on failure and false on success, so we negate its result.
    name += DYLIB_EXTENSION;
    if(!llvm::sys::DynamicLibrary::LoadLibraryPermanently(name.c_str()))
        return true;

    for(const std::string &path : g_additional_paths)
    {
        std::string full_name = path + "/" + name;
        if(llvm::sys::fs::exists(full_name))
            return !llvm::sys::DynamicLibrary::LoadLibraryPermanently(full_name.c_str());
    }
    return false;
}
