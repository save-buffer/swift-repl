#ifndef JIT_H
#define JIT_H

#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>

namespace orc = llvm::orc;

class JIT
{
public:
    static llvm::Expected<std::unique_ptr<JIT>> Create();
    llvm::Error AddModule(std::unique_ptr<llvm::Module> module);
    llvm::Expected<llvm::JITEvaluatedSymbol> LookupSymbol(llvm::StringRef symbol_name);

private:
    JIT(orc::JITTargetMachineBuilder machine_builder, llvm::DataLayout data_layout);

    orc::ExecutionSession m_execution_session;
    orc::RTDyldObjectLinkingLayer m_object_layer;
    orc::IRCompileLayer m_compile_layer;

    llvm::DataLayout m_data_layout;
    orc::MangleAndInterner m_mangler;
    orc::ThreadSafeContext m_ctx;
};
#endif
