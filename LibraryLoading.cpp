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
