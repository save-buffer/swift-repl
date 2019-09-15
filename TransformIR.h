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
