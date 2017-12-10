#pragma once


#include <string>
#include <vector>
#include <memory>


#include "llvm/IR/Module.h"

#include "llvm/Linker/Linker.h"


// For setting target
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/CodeGen/CommandFlags.h"

// For passes
#include "llvm/IR/PassManager.h

// For compilation
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/Object/ObjectFile.h"



#include "supportingClasses.h
#include "MemoryManager.h"

// temporary, mock
#include "IDSLCallable.h"


namespace hetarch {


class CodeGen {

    const std::string targetName;

//    std::string targetLookupError;

    // todo: do i need to (resource-)manage this?
//    llvm::Target *target{nullptr};
//    const std::unique_ptr<llvm::Target> target;
public:

    explicit CodeGen(const std::string &targetName);

    template<typename RetT, typename... Args>
    ObjCode<RetT, Args...> compile(const IRModule<RetT, Args...> &irModule);


    template<typename RetT, typename... Args>
    static IRModule<RetT, Args...> generateIR(DSLFunction<RetT, Args...> dslFunction);

    // todo: change signature to accepting base class
    template<typename RetT, typename... Args>
    static bool link(IRModule<RetT, Args...> &dependent);

    template<typename RetT, typename... Args>
    static bool link(IRModule<RetT, Args...> &dest, std::vector<IIRModule> &sources);
    CodeGen(const CodeGen &) = delete;
    CodeGen &operator=(const CodeGen &) = delete;

private:
    static bool linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies);


};

CodeGen::CodeGen(const std::string &targetName)
: targetName(llvm::Triple::normalize(targetName))
//, target(std::unique_ptr(llvm::TargetRegistry::lookupTarget(this->targetName, targetLookupError)))
{
    // Initialise targets
    // todo: optimise somehow? (don't init ALL targets - check how expensive it is)
    InitializeAllTargetInfos();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();
    InitializeAllAsmParsers();

    // Initialize default, common passes
    auto passRegistry = llvm::PassRegistry::getPassRegistry();
    initializeCore(*passRegistry);
    initializeCodeGen(*passRegistry);

}



template<typename RetT, typename... Args>
ObjCode<RetT, Args...> CodeGen::compile(const IRModule<RetT, Args...> &irModule) {

    std::string targetLookupError;
    auto target = llvm::TargetRegistry::lookupTarget(this->targetName, targetLookupError);
    if (target) {

        // CPU
        std::string cpuStr = "generic";
        std::string featuresStr = "";

        // todo: do something with options
        llvm::TargetOptions options = InitTargetOptionsFromCodeGenFlags();

        llvm::Reloc::Model relocModel = llvm::Reloc::PIC_;
//        auto codeModel = llvm::CodeModel::Default;

//        auto optLevel = llvm::CodeGenOpt::Default;

        auto targetMachine = target->createTargetMachine(
                this->targetName,
                cpuStr,
                featuresStr,
                options,
                relocModel
        );

        if (targetMachine) {
            // todo: violating constness of irModule???
            irModule.m->setDataLayout(targetMachine->createDataLayout());
            irModule.m->setTargetTriple(this->targetName);

            // There go various Passes with PassManager
            {
                // Set Module-specific passes through PassManager
                // todo: provide debug parameter to constructor
                auto passManager = llvm::PassManager();
            }

            auto compiler = llvm::orc::SimpleCompiler(*targetMachine);
            // seems, specific ObjectFile type (e.g. ELF32) and endianness are determined from TargetMachine
            object::OwningBinary<object::ObjectFile> objFile = compiler(*irModule.m);
//            object::ELFObjectFile

            if (objFile.getBinary()) {
                ; // continue something

//                return ObjCode(objFile);
            } else {
                // todo: handle compilation error
            }
        } else {
            // todo: handle TargetMachine creation error
        }
    } else {
        // todo: handle error: target not found (err message in 'err')
    }
}


// todo: traverse the dependent modules in __some sane order__ for linking
//  generally, it is DAG, but it should be checked (look for cycles while traversing)
bool CodeGen::linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies) {
    for (IIRModule &src : dependencies) {

        // currently - DFS without checks for loops
        // note: Passing OverrideSymbols (in flags) as true will have symbols from Src shadow those in the Dest.
        if (bool isSuccess = !linker.linkInModule(std::move(src.m))) {
            return CodeGen::linkInModules(linker, src.m_dependencies);
        } else {
            // todo: handle link errors (if possible), propagate otherwise
            return isSuccess;
        }
    }
}

template<typename RetT, typename... Args>
bool CodeGen::link(IRModule<RetT, Args...> &dependent) {
    llvm::Linker linker(*dependent.m);

    return CodeGen::linkInModules(linker, dependent.m_dependencies);
}

template<typename RetT, typename... Args>
bool CodeGen::link(IRModule<RetT, Args...> &dest, std::vector<IIRModule> &sources) {
    llvm::Linker linker(*dest.m);

    if (bool isSuccess = !CodeGen::linkInModules(linker, dest.m_dependencies)) {
        return CodeGen::linkInModules(linker, sources);
    } else {
        return isSuccess;
    }
}

}

