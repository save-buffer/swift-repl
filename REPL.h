#ifndef REPL_H
#define REPL_H

#include <iostream>
#include <string>

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

struct REPL
{
    REPL();
    bool IsExitString(const std::string &line);
    bool ExecuteSwift(std::string line);
    
private:
    struct ReplInput
    {
        unsigned buffer_id;
        std::string module_name;
        std::string text;
    };
    
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
            std::string diagnostic;
            llvm::raw_string_ostream stream(diagnostic);
            swift::DiagnosticEngine::formatDiagnosticText(stream, fmt_str, fmt_args);
            stream.flush();
            std::cout << "Received Diagnostic: " << diagnostic << std::endl;
        }
    };
    
    swift::CompilerInvocation m_invocation;
    
    swift::LangOptions m_lang_opts;
    swift::SearchPathOptions m_spath_opts;

    swift::SourceManager m_src_mgr;
    swift::DiagnosticEngine m_diagnostic_engine;
    PrinterDiagnosticConsumer m_diagnostic_consumer;

    llvm::LLVMContext m_llvm_ctx;

    std::unique_ptr<swift::ASTContext> m_ast_ctx;
};
#endif
