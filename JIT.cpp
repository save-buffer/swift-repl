/**
 * Copyright (c) 2019-present, Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "JIT.h"
#include "Logging.h"
#include "Config.h"
#include "LibraryLoading.h"

#include <vector>

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

void JIT::AddSearchPath(std::string path)
{
    SetCurrentLoggingArea(LoggingArea::JIT);
    Log(std::string("Adding ") + path + " to dynamic library search path");
    AddLibrarySearchPath(path);
}

void JIT::AddModule(std::unique_ptr<llvm::Module> module)
{
    llvm::cantFail(m_compile_layer.add(m_execution_session.getMainJITDylib(),
                                       orc::ThreadSafeModule(std::move(module), m_ctx)));
}

bool JIT::AddDylib(std::string name)
{
    SetCurrentLoggingArea(LoggingArea::JIT);
    bool result = SearchForAndLoadLibraryPermanently(name);

    if(result)
        Log(std::string("Successfully loaded dynamic library ") + name);
    else
        Log(std::string("Failed to load dynamic library ") + name, LoggingPriority::Warning);
    return result;
}

llvm::Expected<llvm::JITEvaluatedSymbol> JIT::LookupSymbol(llvm::StringRef symbol_name)
{
    SetCurrentLoggingArea(LoggingArea::JIT);
    Log((llvm::Twine("Looking up symbol ") + symbol_name).str());
    return m_execution_session.lookup({ &m_execution_session.getMainJITDylib() },
                                      m_mangler(symbol_name.str()));
}

llvm::Error JIT::RemoveSymbol(llvm::StringRef symbol_name)
{
    SetCurrentLoggingArea(LoggingArea::JIT);
    Log(std::string("Removing symbol ") + symbol_name.str());
    return m_execution_session.getMainJITDylib().remove({ m_execution_session.intern(symbol_name.str()) });
}

orc::SymbolNameSet JIT::SymbolGenerator::operator()(orc::JITDylib &jd, const orc::SymbolNameSet &names)
{
    orc::SymbolNameSet imps;
    orc::SymbolNameSet non_imps;
    for(auto &name : names)
    {
        if((*name).startswith("__imp_"))
            imps.insert(name);
        else
            non_imps.insert(name);
    }

    orc::SymbolNameSet added = m_search(jd, non_imps);
    orc::SymbolMap new_symbols;

    for(auto &_imp : imps)
    {
        auto imp = *_imp;
        llvm::StringRef base = imp.substr(strlen("__imp_"));
        if(auto symb = m_jit.LookupSymbol(base))
        {
            std::uintptr_t address = reinterpret_cast<std::uintptr_t>(symb->getAddress());
            m_imps.push_back(new std::uintptr_t(address));

            auto interned_imp = m_jit.m_execution_session.intern(imp.str());
            new_symbols[interned_imp] =
                llvm::JITEvaluatedSymbol(llvm::pointerToJITTargetAddress(m_imps.back()),
                                        llvm::JITSymbolFlags::Exported);
            added.insert(interned_imp);
        }
    }
    if(!new_symbols.empty())
        cantFail(jd.define(orc::absoluteSymbols(std::move(new_symbols))));
    return added;
}

JIT::SymbolGenerator::~SymbolGenerator()
{
    for(std::uintptr_t *p : m_imps)
        delete p;
}

JIT::JIT(orc::JITTargetMachineBuilder machine_builder,
         llvm::DataLayout data_layout) : m_object_layer(m_execution_session,
                                                        []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
                                         m_compile_layer(m_execution_session,
                                                         m_object_layer,
                                                         orc::ConcurrentIRCompiler(std::move(machine_builder))),
                                         m_data_layout(std::move(data_layout)),
                                         m_mangler(m_execution_session, m_data_layout),
                                         m_ctx(std::make_unique<llvm::LLVMContext>()),
                                         m_generator(*this, m_data_layout)
{
    m_execution_session.getMainJITDylib().setGenerator(m_generator);
    m_object_layer.setOverrideObjectFlagsWithResponsibilityFlags(true);
    m_object_layer.setAutoClaimResponsibilityForObjectSymbols(true);
}
