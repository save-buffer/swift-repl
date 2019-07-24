#include "REPL.h"
#include "TransformAST.h"

#define SWIFT_MODULE_PATH "S:\\b\\swift\\lib\\swift\\windows\\x86_64"

void ConfigureFunctionLinkage(std::unique_ptr<swift::SILModule> &sil_module)
{
    for(auto &fn : sil_module->getFunctionList())
    {
        fn.setLinkage(swift::SILLinkage::Public);
        std::cout << "Set SIL Function " << fn.getName().str() << " to public\n";
    }

    sil_module->lookUpFunction("main")->setLinkage(swift::SILLinkage::Private);
}

REPL::REPL() : m_diagnostic_engine(m_src_mgr),
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

bool REPL::IsExitString(const std::string &line)
{
    return line == "e" || line == "exit";
}

bool REPL::ExecuteSwift(std::string line)
{
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

    constexpr auto src_file_kind = swift::SourceFileKind::Main;
    swift::SourceFile *src_file = new (*m_ast_ctx) swift::SourceFile(*module,
                                                                     src_file_kind,
                                                                     input.buffer_id,
                                                                     implicit_import_kind,
                                                                     false);
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
    std::cout << "=========AST==========\n";
    src_file->dump();
    
    //TODO(sasha): Load DLLs
    swift::performNameBinding(*src_file);
    CHECK_ERROR();
    swift::TopLevelContext top_level_context;
    swift::OptionSet<swift::TypeCheckingFlags> type_check_opts;
    swift::performTypeChecking(*src_file, top_level_context, type_check_opts);
    
    ModifyAST(*src_file);
    
    std::cout << "=========AST==========\n";
    src_file->dump();

    //TODO(sasha): Perform playground transform?
    CHECK_ERROR();
    swift::typeCheckExternalDefinitions(*src_file);
    CHECK_ERROR();
    //TODO(sasha): Remember variables from previous expressions?

    std::cout << "=========AST==========\n";
    src_file->dump();
    
    std::unique_ptr<swift::SILModule> sil_module(
        swift::performSILGeneration(*src_file,
                                    m_invocation.getSILOptions()));

    ConfigureFunctionLinkage(sil_module);
    
    std::cout << "=========SIL==========\n";
    sil_module->dump();
    
    std::unique_ptr<llvm::Module> llvm_module(swift::performIRGeneration(m_invocation.getIRGenOptions(),
                                                                         *src_file,
                                                                         std::move(sil_module),
                                                                         "swift_repl_module",
                                                                         swift::PrimarySpecificPaths("", input.module_name),
                                                                         m_llvm_ctx));
    std::cout << "=========LLVM IR==========\n";
    std::string llvm_ir;
    llvm::raw_string_ostream str_stream(llvm_ir);
    llvm_module->print(str_stream, nullptr);
    str_stream.flush();
    std::cout << llvm_ir << std::endl;

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
    TransformFinalExpressionAndAddGlobal(src_file);
    WrapInFunction(src_file);
    MakeDeclarationsPublic(src_file);
}

REPL::ReplInput REPL::AddToSrcMgr(const std::string &line)
{
    ReplInput result;
    static uint64_t s_num_inputs = 0;
    result.text = line;
    llvm::raw_string_ostream stream(result.module_name);
    stream << "__repl_" << (s_num_inputs++);
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
