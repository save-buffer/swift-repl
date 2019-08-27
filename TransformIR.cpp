#include "TransformIR.h"
#include "Logging.h"

#include <algorithm>

#include <llvm/IR/Instructions.h>

#define FN_PTR_SUFFIX "_ptr"

void AddFunctionPointers(std::unique_ptr<llvm::Module> &llvm_module,
                         std::unique_ptr<JIT> &jit,
                         llvm::LLVMContext &llvm_ctx,
                         std::unordered_map<std::string, std::string> &fn_ptr_map)
{
    auto ptr_module = std::make_unique<llvm::Module>("fn_ptrs", llvm_ctx);
    for(auto &fn : llvm_module->getFunctionList())
    {
        assert(fn.hasExternalLinkage());

        std::string fn_name = fn.getName().str();
        std::string ptr_name = fn_name + FN_PTR_SUFFIX;
        if(fn_ptr_map.find(fn_name) == fn_ptr_map.end())
            continue;

        if(fn_ptr_map[fn_name].empty())
        {
            llvm::Type *ptr_type = llvm::Type::getInt8PtrTy(llvm_ctx);
            llvm::GlobalVariable *ptr = new llvm::GlobalVariable(*ptr_module,
                                                                 ptr_type,
                                                                 false,
                                                                 llvm::GlobalValue::LinkageTypes::ExternalLinkage,
                                                                 llvm::Constant::getNullValue(ptr_type),
                                                                 ptr_name);
            fn_ptr_map[fn_name] = ptr_name;
        }
        assert(fn_ptr_map[fn_name] == ptr_name);
    }
    jit->AddModule(std::move(ptr_module));
}

void ReplaceFunctionCallsWithIndirectFunctionCalls(
    llvm::Instruction &i, std::unique_ptr<llvm::Module> &module,
    llvm::LLVMContext &llvm_ctx,
    std::unordered_map<std::string, std::string> &fn_ptr_map)
{
    SetCurrentLoggingArea(LoggingArea::IR);
    auto *call_inst = llvm::dyn_cast<llvm::CallInst>(&i);
    if(!call_inst)
        return;

    auto *called_fn = call_inst->getCalledFunction();
    if(!called_fn)
        return;

    std::string fn_name = called_fn->getName().str();
    if(fn_ptr_map.find(fn_name) == fn_ptr_map.end())
        return;

    std::string log_string;
    llvm::raw_string_ostream str_stream(log_string);
    str_stream << "Changed instruction:\n\t";
    call_inst->print(str_stream);

    std::string ptr_name = fn_ptr_map[fn_name];
    llvm::GlobalVariable *ptr = module->getGlobalVariable(ptr_name);
    if(!ptr)
    {
        ptr = new llvm::GlobalVariable(
            *module, llvm::Type::getInt8PtrTy(llvm_ctx),
            false, llvm::GlobalValue::LinkageTypes::ExternalLinkage,
            nullptr, ptr_name);
        ptr->setDLLStorageClass(llvm::GlobalValue::DLLStorageClassTypes::DLLImportStorageClass);
    }

    auto *load = new llvm::LoadInst(ptr->getType()->getElementType(), ptr);
    auto *bitcast = new llvm::BitCastInst(load, called_fn->getFunctionType()->getPointerTo(), "", call_inst);
    load->insertBefore(bitcast);
    call_inst->setCalledOperand(bitcast);

    str_stream << "\nTo:\n\t";
    load->print(str_stream);
    str_stream << "\n\t";
    bitcast->print(str_stream);
    str_stream << "\n\t";
    call_inst->print(str_stream);
    str_stream << "\nAdded global:\n\t";
    ptr->print(str_stream);
    str_stream.flush();
    Log(log_string);
}

void ReplaceFunctionCallsWithIndirectFunctionCalls(
    llvm::BasicBlock &bb, std::unique_ptr<llvm::Module> &module,
    llvm::LLVMContext &llvm_ctx,
    std::unordered_map<std::string, std::string> &fn_ptr_map)
{
    std::for_each(bb.begin(), bb.end(),
                  [&](auto &i)
                  {
                      ReplaceFunctionCallsWithIndirectFunctionCalls(i, module, llvm_ctx, fn_ptr_map);
                  });
}

void ReplaceFunctionCallsWithIndirectFunctionCalls(
    llvm::Function &fn, std::unique_ptr<llvm::Module> &module,
    llvm::LLVMContext &llvm_ctx,
    std::unordered_map<std::string, std::string> &fn_ptr_map)
{
    std::for_each(fn.getBasicBlockList().begin(), fn.getBasicBlockList().end(),
                  [&](auto &bb)
                  {
                      ReplaceFunctionCallsWithIndirectFunctionCalls(bb, module, llvm_ctx, fn_ptr_map);
                  });
}

void ReplaceFunctionCallsWithIndirectFunctionCalls(
    std::unique_ptr<llvm::Module> &module, llvm::LLVMContext &llvm_ctx,
    std::unordered_map<std::string, std::string> &fn_ptr_map)
{
    std::for_each(module->getFunctionList().begin(), module->getFunctionList().end(),
                  [&](auto &fn)
                  {
                      ReplaceFunctionCallsWithIndirectFunctionCalls(fn, module, llvm_ctx, fn_ptr_map);
                  });
}
