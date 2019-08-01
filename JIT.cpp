#include "JIT.h"
#include "Logging.h"

llvm::Expected<std::unique_ptr<JIT>> JIT::Create()
{
    auto machine_builder = orc::JITTargetMachineBuilder::detectHost();
    if(!machine_builder)
        return machine_builder.takeError();
    
    auto data_layout = machine_builder->getDefaultDataLayoutForTarget();
    if(!data_layout)
        return data_layout.takeError();
    
    return std::unique_ptr<JIT>(new JIT(std::move(*machine_builder), std::move(*data_layout)));
}

void JIT::AddModule(std::unique_ptr<llvm::Module> module)
{
    llvm::cantFail(m_compile_layer.add(m_execution_session.getMainJITDylib(),
                                       orc::ThreadSafeModule(std::move(module), m_ctx)));
}

llvm::Expected<llvm::JITEvaluatedSymbol> JIT::LookupSymbol(llvm::StringRef symbol_name)
{
    SetCurrentLoggingArea(LoggingArea::JIT);
    Log(std::string("Looking up symbol ") + symbol_name.str());
    return m_execution_session.lookup({ &m_execution_session.getMainJITDylib() },
                                      m_mangler(symbol_name.str()));
}

JIT::JIT(orc::JITTargetMachineBuilder machine_builder,
         llvm::DataLayout data_layout) : m_object_layer(m_execution_session,
                                                        []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
                                         m_compile_layer(m_execution_session,
                                                         m_object_layer,
                                                         orc::ConcurrentIRCompiler(std::move(machine_builder))),
                                         m_data_layout(std::move(data_layout)),
                                         m_mangler(m_execution_session, m_data_layout),
                                         m_ctx(std::make_unique<llvm::LLVMContext>())
{
    static bool s_run_guard = false;
    if(!s_run_guard)
    {
        s_run_guard = true;
        llvm::sys::DynamicLibrary::LoadLibraryPermanently("S:\\b\\swift\\bin\\swiftCore.dll");
    }

    m_execution_session.getMainJITDylib().setGenerator(
        llvm::cantFail(orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(data_layout)));
    m_object_layer.setOverrideObjectFlagsWithResponsibilityFlags(true);
}
