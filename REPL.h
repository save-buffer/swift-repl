#ifndef REPL_H
#define REPL_H

#include <iostream>
#include <string>
#include <unordered_map>

#include <llvm/IR/Module.h>

#include <swift/Subsystems.h>
#include <swift/AST/ASTContext.h>
#include <swift/AST/DiagnosticConsumer.h>
#include <swift/AST/DiagnosticEngine.h>
#include <swift/Basic/LangOptions.h>
#include <swift/Basic/LLVMInitialize.h>
#include <swift/Basic/SourceManager.h>
#include <swift/ClangImporter/ClangImporter.h>
#include <swift/Frontend/Frontend.h>
#include <swift/Frontend/ParseableInterfaceModuleLoader.h>
#include <swift/SIL/SILModule.h>

#include "Config.h"
#include "JIT.h"

struct REPL
{
    static llvm::Expected<std::unique_ptr<REPL>> Create(
        bool is_playground = false,
        std::string default_module_cache_path = DEFAULT_MODULE_CACHE_PATH);
    std::string GetLine();
    void AddModuleSearchPath(std::string path);
    void AddLoadSearchPath(std::string path);
    bool IsExitString(const std::string &line);
    bool ExecuteSwift(std::string line);

protected:
    explicit REPL(bool is_playground, std::string default_module_cache_path);

private:
    struct ReplInput
    {
        unsigned buffer_id;
        std::string module_name;
        std::string text;
    };

    void RemoveRedeclarationsFromJIT(std::unique_ptr<llvm::Module> &sil_module);
    llvm::Error UpdateFunctionPointers();
    bool CompileSourceFileToIRAndAddToJIT(swift::SourceFile &src_file);
    void LoadImportedModules(swift::SourceFile &src_file);
    void ModifyAST(swift::SourceFile &src_file);
    ReplInput AddToSrcMgr(const std::string &line);
    void SetupLangOpts();
    void SetupSearchPathOpts();
    void SetupSILOpts();
    void SetupIROpts();
    void SetupImporters();

    class PrinterDiagnosticConsumer : public swift::DiagnosticConsumer
    {
        void handleDiagnostic(swift::SourceManager &src_mgr,
                              swift::SourceLoc loc,
                              swift::DiagnosticKind kind,
                              llvm::StringRef fmt_str,
                              llvm::ArrayRef<swift::DiagnosticArgument> fmt_args,
                              const swift::DiagnosticInfo &info,
                              const swift::SourceLoc)
        {
            //NOTE(sasha): We don't use normal logging system here because we always
            //             want to show compiler errors.
            std::string diagnostic;
            llvm::raw_string_ostream stream(diagnostic);
            swift::DiagnosticEngine::formatDiagnosticText(stream, fmt_str, fmt_args);
            stream.flush();
            std::cout << diagnostic << std::endl;
        }
    };

    const bool m_is_playground;
    const std::string m_default_module_cache_path;
    uint64_t m_curr_input_number;

    swift::CompilerInvocation m_invocation;
    
    swift::LangOptions m_lang_opts;
    swift::SearchPathOptions m_spath_opts;

    swift::SourceManager m_src_mgr;
    swift::DiagnosticEngine m_diagnostic_engine;
    PrinterDiagnosticConsumer m_diagnostic_consumer;

    llvm::LLVMContext m_llvm_ctx;

    std::unique_ptr<swift::ASTContext> m_ast_ctx;

    using DeclMap = std::unordered_map<std::string, swift::SourceFile *>;
    DeclMap m_decl_map;
    std::unordered_map<std::string, std::string> m_fn_ptr_map;
    std::vector<swift::ImportDecl *> m_imports;

    std::unique_ptr<JIT> m_jit;
};
#endif
