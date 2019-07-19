#include <iostream>
#include <string>

#include <swift/Subsystems.h>
#include <swift/AST/ASTContext.h>
#include <swift/AST/DiagnosticConsumer.h>
#include <swift/AST/DiagnosticEngine.h>
#include <swift/Basic/LangOptions.h>
#include <swift/Basic/SourceManager.h>
#include <swift/ClangImporter/ClangImporter.h>
#include <swift/Frontend/Frontend.h>
#include <swift/Frontend/ParseableInterfaceModuleLoader.h>

#define SWIFT_MODULE_PATH "S:\\b\\swift\\lib\\swift\\windows\\x86_64"

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

    void SetupImporters()
    {
        swift::DependencyTracker *tracker = nullptr;
        std::string module_cache_path;
        swift::ClangImporterOptions &clang_importer_opts =
            m_invocation.getClangImporterOptions();
        clang_importer_opts.OverrideResourceDir = "S:\\b\\swift\\lib\\swift\\clang";
        std::unique_ptr<swift::ClangImporter> clang_importer;
        if(!clang_importer_opts.OverrideResourceDir.empty())
        {
            clang_importer = swift::ClangImporter::create(*m_ast_ctx, clang_importer_opts);
            if(!clang_importer)
            {
                std::cout << "Failed to create ClangImporter" << std::endl;
            }
            else
            {
                clang_importer->addSearchPath("S:\\b\\swift\\lib\\swift\\shims", /* isFramework */ false, /* ifSystem */ true);
                module_cache_path = swift::getModuleCachePathFromClang(clang_importer->getClangInstance());
                std::cout << module_cache_path << std::endl;
            }
        }
        if(module_cache_path.empty())
        {
            llvm::SmallString<0> path;
            std::error_code ec = llvm::sys::fs::createUniqueDirectory("ModuleCache", path);
            if(!ec)
                module_cache_path = path.str();
            else
                module_cache_path = "C:\\Windows\\Temp\\SwiftModuleCache";
        }
        constexpr swift::ModuleLoadingMode loading_mode = swift::ModuleLoadingMode::PreferSerialized;
        llvm::Triple triple(m_invocation.getTargetTriple());
        std::string prebuilt_module_cache_path = SWIFT_MODULE_PATH;
        auto memory_buffer_loader(swift::MemoryBufferSerializedModuleLoader::create(*m_ast_ctx,
                                                                                    tracker,
                                                                                    swift::ModuleLoadingMode::PreferSerialized)
            );
        auto stdlib_buffer = llvm::MemoryBuffer::getFile(SWIFT_MODULE_PATH "\\Swift.swiftmodule");
        if(stdlib_buffer)
            memory_buffer_loader->registerMemoryBuffer("Swift", std::move(stdlib_buffer.get()));
        else
            std::cerr << "Failed to load module Swift\n";

        if(memory_buffer_loader)
            m_ast_ctx->addModuleLoader(std::move(memory_buffer_loader));
        else
            std::cout << "Memory buffer loader failed\n";

        if(loading_mode != swift::ModuleLoadingMode::OnlySerialized)
        {
            std::unique_ptr<swift::ModuleLoader> parseable_module_loader(
                swift::ParseableInterfaceModuleLoader::create(*m_ast_ctx,
                                                              module_cache_path,
                                                              prebuilt_module_cache_path,
                                                              tracker,
                                                              loading_mode)
                );
            if(parseable_module_loader)
                m_ast_ctx->addModuleLoader(std::move(parseable_module_loader));
        }
        std::unique_ptr<swift::ModuleLoader> serialized_module_loader(
            swift::SerializedModuleLoader::create(*m_ast_ctx, tracker, loading_mode)
            );

        if(serialized_module_loader)
            m_ast_ctx->addModuleLoader(std::move(serialized_module_loader));

        if(clang_importer)
            m_ast_ctx->addModuleLoader(std::move(clang_importer), /* isClang = */ true);

        // NOTE(sasha): LLDB installs a DWARF importer here. We don't care about that (or do we?)
    }

    swift::CompilerInvocation m_invocation;
    
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
