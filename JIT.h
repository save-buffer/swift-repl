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
    void AddSearchPath(std::string path);
    void AddModule(std::unique_ptr<llvm::Module> module);
    bool AddDylib(std::string absolute_path);
    llvm::Expected<llvm::JITEvaluatedSymbol> LookupSymbol(llvm::StringRef symbol_name);
    // NOTE(sasha): Returns SymbolsNotFound Error if the symbol was not found
    //              Returns SymbolsCouldNotBeRemoved on failure to actually remove the symbol
    //              Returns Error::success() on success, does nothing on failure.
    llvm::Error RemoveSymbol(llvm::StringRef symbol_name);

private:
    class SymbolGenerator
    {
    public:
        SymbolGenerator(JIT &jit, llvm::DataLayout data_layout)
            : m_jit(jit),
              m_search(llvm::cantFail(
                           orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
                               data_layout))) {}
        orc::SymbolNameSet operator()(orc::JITDylib &jd, const orc::SymbolNameSet &names);
        ~SymbolGenerator();
    private:
        orc::DynamicLibrarySearchGenerator m_search;
        std::vector<std::uintptr_t *> m_imps;
        JIT &m_jit;
    };

    JIT(orc::JITTargetMachineBuilder machine_builder, llvm::DataLayout data_layout);

    orc::ExecutionSession m_execution_session;
    orc::RTDyldObjectLinkingLayer m_object_layer;
    orc::IRCompileLayer m_compile_layer;

    llvm::DataLayout m_data_layout;
    orc::MangleAndInterner m_mangler;
    orc::ThreadSafeContext m_ctx;

    SymbolGenerator m_generator;
};
#endif
