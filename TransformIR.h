#ifndef TRANSFORMIR_H
#define TRANSFORMIR_H

#include "JIT.h"

#include <cstdint>
#include <string>
#include <unordered_map>

#include <llvm/IR/Module.h>

// Adds the function pointers to the JIT corresponding to functions in fn_ptr_map that have
// an empty pointer name
void AddFunctionPointers(
    std::unique_ptr<llvm::Module> &module, std::unique_ptr<JIT> &jit,
    llvm::LLVMContext &llvm_ctx,
    std::unordered_map<std::string, std::string> &fn_ptr_map);

void ReplaceFunctionCallsWithIndirectFunctionCalls(
    std::unique_ptr<llvm::Module> &module, llvm::LLVMContext &llvm_ctx,
    std::unordered_map<std::string, std::string> &fn_ptr_map);
#endif
