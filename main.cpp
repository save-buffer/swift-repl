#include <iostream>
#include <string>

#include <swift/AST/ASTContext.h>
#include <swift/AST/DiagnosticConsumer.h>
#include <swift/AST/DiagnosticEngine.h>
#include <swift/Basic/LangOptions.h>
#include <swift/Basic/SourceManager.h>

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

struct REPL
{
    REPL() : m_diagnostic_engine(m_src_mgr),
             m_ast_ctx(swift::ASTContext::get(m_lang_opts,
                                              m_spath_opts,
                                              m_src_mgr,
                                              m_diagnostic_engine))
    {
        m_diagnostic_engine.setShowDiagnosticsAfterFatalError();
        m_diagnostic_engine.addConsumer(m_diagnostic_consumer);

        SetupLangOpts();
        SetupSearchPathOpts();
    }

    bool IsExitString(const std::string &line)
    {
        return line == "e" || line == "exit";
    }

    bool ExecuteSwift(std::string line)
    {
        if(IsExitString(line))
            return false;
        return true;
    }

private:
    void SetupLangOpts()
    {
        m_lang_opts.Target.setArch(llvm::Triple::ArchType::x86_64);
        m_lang_opts.Target.setOS(llvm::Triple::OSType::Win32);
        m_lang_opts.Target.setEnvironment(llvm::Triple::EnvironmentType::MSVC);
        m_lang_opts.Target.setObjectFormat(llvm::Triple::ObjectFormatType::COFF);
        m_lang_opts.EnableDollarIdentifiers = true;
        m_lang_opts.EnableAccessControl = true;
        m_lang_opts.EnableTargetOSChecking = false;
        m_lang_opts.Playground = true;
        m_lang_opts.EnableThrowWithoutTry = true;
    }

    void SetupSearchPathOpts()
    {
        m_spath_opts.RuntimeResourcePath = "S:\\b\\swift\\lib\\swift";
        m_spath_opts.SDKPath = "C:\\Library\\Developer\\Platforms\\Windows.platform\\Developer\\SDKs\\Windows.sdk";
    }

    swift::LangOptions m_lang_opts;
    swift::SearchPathOptions m_spath_opts;

    swift::SourceManager m_src_mgr;
    swift::DiagnosticEngine m_diagnostic_engine;
    PrinterDiagnosticConsumer m_diagnostic_consumer;

    std::unique_ptr<swift::ASTContext> m_ast_ctx;
};

int main(int argc, char **argv)
{
    std::string line;
    REPL repl;
    do
    {
        std::getline(std::cin, line);
    } while(repl.ExecuteSwift(line));
    return 0;
}
