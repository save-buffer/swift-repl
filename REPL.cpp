#include "REPL.h"
#include "Logging.h"
#include "TransformAST.h"

#include <cstdint>
#include <tuple>
#include <type_traits>

#include <swift/AST/ASTMangler.h>
#include <swift/SILOptimizer/PassManager/Passes.h>

#define SWIFT_MODULE_PATH "S:\\b\\swift\\lib\\swift\\windows\\x86_64"

// Returns expression function and result global variable
swift::FuncDecl *ConfigureFunctionLinkage(swift::SourceFile &src_file, std::unique_ptr<swift::SILModule> &sil_module)
{
    SetCurrentLoggingArea(LoggingArea::SIL);

    swift::FuncDecl *res_fn = nullptr;

    const std::string res_fn_name = src_file.getFilename().str();
    for(swift::Decl *decl : src_file.Decls)
    {
        assert(decl);
        if(auto fn = llvm::dyn_cast<swift::FuncDecl>(decl))
        {
            swift::SILDeclRef sil_decl(fn);

            std::string name_original = fn->getName().str();
            std::string name_mangled = sil_decl.mangle();

            sil_module->lookUpFunction(sil_decl)->setLinkage(swift::SILLinkage::Public);
            Log(std::string("Set function ") + name_original + " (" + name_mangled + ") to public");

            if(name_original == res_fn_name)
                res_fn = fn;
        }
    }
    sil_module->lookUpFunction("main")->setLinkage(swift::SILLinkage::Private);
    return res_fn;
}

llvm::Expected<std::unique_ptr<REPL>> REPL::Create()
{
    std::unique_ptr<REPL> result(new REPL);
    auto jit = JIT::Create();
    SetCurrentLoggingArea(LoggingArea::JIT);
    if(!jit)
    {
        Log("Failed to initialize JIT", LoggingPriority::Error);
        return jit.takeError();
    }
    result->m_jit = std::move(*jit);
    return std::unique_ptr<REPL>(std::move(result));
}

REPL::REPL() : m_curr_input_number(0),
               m_diagnostic_engine(m_src_mgr),
               m_ast_ctx(swift::ASTContext::get(m_lang_opts,
                                                m_spath_opts,
                                                m_src_mgr,
                                                m_diagnostic_engine))
{
    static bool s_run_guard = false;
    if(!s_run_guard)
    {
        INITIALIZE_LLVM();
        s_run_guard = true;
    }

    m_diagnostic_engine.setShowDiagnosticsAfterFatalError();
    m_diagnostic_engine.addConsumer(m_diagnostic_consumer);

    SetupLangOpts();
    SetupSearchPathOpts();
    SetupSILOpts();
    SetupIROpts();
    SetupImporters();
    swift::registerTypeCheckerRequestFunctions(m_ast_ctx->evaluator);
}

std::string REPL::GetLine()
{
    m_curr_input_number++;
    std::cout << "\n";
    std::string result = "";
    do
    {
        std::cout << m_curr_input_number << "> ";
        std::getline(std::cin, result);
    } while(result.empty());
    return result;
}

bool REPL::IsExitString(const std::string &line)
{
    return line == "e" || line == "exit";
}

bool REPL::ExecuteSwift(std::string line)
{
    //NOTE(sasha): We don't use the normal logging system here because the
    //             DiagnosticEngine will show have showed the error.
#define CHECK_ERROR() if(m_diagnostic_engine.hadAnyError()) { return true; }
    m_diagnostic_engine.resetHadAnyError();

    if(IsExitString(line))
        return false;

    ReplInput input = AddToSrcMgr(line);
    auto module_id = m_ast_ctx->getIdentifier(input.module_name);
    auto *module = swift::ModuleDecl::create(module_id, *m_ast_ctx);
    CHECK_ERROR();
    constexpr auto implicit_import_kind =
        swift::SourceFile::ImplicitModuleImportKind::Stdlib;
    m_invocation.getFrontendOptions().ModuleName = input.module_name.c_str();
    m_invocation.getIRGenOptions().ModuleName = input.module_name.c_str();

    swift::SourceFile *src_file = new (*m_ast_ctx) swift::SourceFile(
        *module, swift::SourceFileKind::Main, input.buffer_id,
        implicit_import_kind);
    module->addFile(*src_file);

    CHECK_ERROR();
    swift::PersistentParserState persistent_state(*m_ast_ctx);

    bool done = false;
    do
    {
        swift::parseIntoSourceFile(*src_file,
                                   input.buffer_id,
                                   &done,
                                   nullptr /* SILParserState */,
                                   &persistent_state,
                                   nullptr /* DelayedParseCB */,
                                   false /* DelayBodyParsing */);
        CHECK_ERROR();
    } while(!done);
    SetCurrentLoggingArea(LoggingArea::AST);
    if(ShouldLog(LoggingPriority::Info))
    {
        Log("=========AST Before Modifications==========");
        src_file->dump();
    }
    AddImportNodes(*src_file, m_modules);
    //TODO(sasha): Load DLLs
    swift::performNameBinding(*src_file);
    CHECK_ERROR();
    swift::TopLevelContext top_level_context;
    swift::OptionSet<swift::TypeCheckingFlags> type_check_opts;
    swift::performTypeChecking(*src_file, top_level_context, type_check_opts);
    
    ModifyAST(*src_file);
    
    //TODO(sasha): Perform playground transform?
    CHECK_ERROR();
    swift::typeCheckExternalDefinitions(*src_file);
    CHECK_ERROR();

    if(ShouldLog(LoggingPriority::Info))
    {
        Log("=========AST After Modification==========");
        src_file->dump();
    }

    std::unique_ptr<swift::SILModule> sil_module(
        swift::performSILGeneration(*src_file,
                                    m_invocation.getSILOptions()));
    CHECK_ERROR();
    swift::FuncDecl *res_fn = ConfigureFunctionLinkage(*src_file, sil_module);
    swift::runSILDiagnosticPasses(*sil_module);
    CHECK_ERROR();
    SetCurrentLoggingArea(LoggingArea::SIL);
    if(ShouldLog(LoggingPriority::Info))
    {
        Log("=========SIL==========");
        sil_module->dump();
    }
    CHECK_ERROR();
    std::unique_ptr<llvm::Module> llvm_module(swift::performIRGeneration(m_invocation.getIRGenOptions(),
                                                                         *src_file,
                                                                         std::move(sil_module),
                                                                         "swift_repl_module",
                                                                         swift::PrimarySpecificPaths("", input.module_name),
                                                                         m_llvm_ctx));

    SetCurrentLoggingArea(LoggingArea::IR);
    if(ShouldLog(LoggingPriority::Info))
    {
        Log("Symbols in IR:");
        for(auto &g : llvm_module->global_values())
            std::cout << '\t' << g.getName().str() << '\n';

        std::string llvm_ir;
        llvm::raw_string_ostream str_stream(llvm_ir);
        str_stream << "=========LLVM IR==========\n";
        llvm_module->print(str_stream, nullptr);
        str_stream.flush();
        Log(llvm_ir);
    }

    SetCurrentLoggingArea(LoggingArea::JIT);
    m_jit->AddModule(std::move(llvm_module));

    if(res_fn)
    {
        swift::Mangle::ASTMangler mangler;
        std::string mangled_fn_name = mangler.mangleEntity(res_fn, false);

        using IntFn = std::add_pointer<uint64_t()>::type;
        IntFn result_fn = nullptr;
        uint64_t *result =  nullptr;
        if(auto symbol = m_jit->LookupSymbol(mangled_fn_name))
        {
            result_fn = reinterpret_cast<IntFn>(symbol->getAddress());
            Log(std::string("Loaded function ") + mangled_fn_name);
        }
        else
        {
            Log(std::string("Failed to load function ") + mangled_fn_name, LoggingPriority::Error);
            return false;
        }
        result_fn();
    }
    m_modules.push_back(module_id);
    m_ast_ctx->LoadedModules[module_id] = module;
    return true;
#undef CHECK_ERROR
}

// ModifyAST performs four modifications on AST:
//    - Add global variable of same type as last expression.
//    - Modify last expression to be assignment to this global variable.
//    - Wrap existing AST into a function called  __repl_x where x is the REPL line number
//      (generated in AddToSrcMgr). We do this so that we don't have to remake the JIT object
//      every time we execute a new REPL line. Later, we lookup the function by name from the
//      JIT and call it and print out result.
//    - Make all declarations public (except classes which will be made open) so that our
//      our function actually gets generated. 
void REPL::ModifyAST(swift::SourceFile &src_file)
{
    CombineTopLevelDeclsAndMoveToBack(src_file);
    TransformFinalExpressionAndAddGlobal(src_file);
    WrapInFunction(src_file);
    MakeDeclarationsPublic(src_file);
}

REPL::ReplInput REPL::AddToSrcMgr(const std::string &line)
{
    ReplInput result;
    result.text = line;
    llvm::raw_string_ostream stream(result.module_name);
    stream << "__repl_" << m_curr_input_number;
    stream.flush();
    std::unique_ptr<llvm::MemoryBuffer> mb = llvm::MemoryBuffer::getMemBufferCopy(line, result.module_name);
    result.buffer_id = m_src_mgr.addNewSourceBuffer(std::move(mb));
    return result;
}

void REPL::SetupLangOpts()
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

void REPL::SetupSearchPathOpts()
{
    m_spath_opts.RuntimeResourcePath = "S:\\b\\swift\\lib\\swift";
    m_spath_opts.SDKPath = "C:\\Library\\Developer\\Platforms\\Windows.platform\\Developer\\SDKs\\Windows.sdk";
}

void REPL::SetupSILOpts()
{
    swift::SILOptions &sil_opts = m_invocation.getSILOptions();
    sil_opts.DisableSILPerfOptimizations = true;
    sil_opts.OptMode = swift::OptimizationMode::NoOptimization;
}

void REPL::SetupIROpts()
{
    swift::IRGenOptions &ir_opts = m_invocation.getIRGenOptions();
    ir_opts.OutputKind = swift::IRGenOutputKind::Module;
    ir_opts.UseJIT = true;
}

void REPL::SetupImporters()
{
    SetCurrentLoggingArea(LoggingArea::Importer);
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
            Log("Failed to create ClangImporter", LoggingPriority::Error);
        }
        else
        {
            clang_importer->addSearchPath("S:\\b\\swift\\lib\\swift\\shims", /* isFramework */ false, /* ifSystem */ true);
            module_cache_path = swift::getModuleCachePathFromClang(clang_importer->getClangInstance());
            Log("Module Cache Path: " + module_cache_path);
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
        Log("Failed to load module Swift", LoggingPriority::Error);

    if(memory_buffer_loader)
        m_ast_ctx->addModuleLoader(std::move(memory_buffer_loader));
    else
        Log("Failed to load module Swift", LoggingPriority::Error);

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
