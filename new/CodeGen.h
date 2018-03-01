#pragma once


#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <utility>


#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"

#include "llvm/Linker/Linker.h"
#include "llvm/Object/ObjectFile.h"

// For setting target
//#include "llvm/ADT/Triple.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/CodeGen/CommandFlags.h"
// For passes
#include "llvm/Passes/PassBuilder.h"
//#include "llvm/IR/PassManager.h"
//#include "llvm/InitializePasses.h"
// For compilation
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/Object/ObjectFile.h"


#include "supportingClasses.h"
#include "dsl/dsl_base.h"
#include "dsl/fun.h"
#include "../tests/test_utils.h"


namespace hetarch {


class CodeGen {
    const std::string targetName;

    const llvm::Target* target{nullptr};
    const llvm::PassRegistry* pm{nullptr};
public:

    explicit CodeGen(const std::string &targetName);

    template<typename RetT, typename... Args>
    ObjCode<RetT, Args...> compile(IRModule<RetT, Args...> &irModule) const;

    static bool link(IIRModule &dependent);

    static bool link(IIRModule &dest, std::vector<IIRModule> &sources);

    CodeGen(const CodeGen &) = delete;
    CodeGen &operator=(const CodeGen &) = delete;

private:
    static bool linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies);

    static std::vector<std::string> findUndefinedSymbols(object::OwningBinary<object::ObjectFile> &objFile);

    static const llvm::Target* initTarget(const std::string& targetName);
    static const llvm::PassRegistry* initPasses();

    void runPasses(llvm::Module& module, llvm::TargetMachine* tm = nullptr) const;
};


template<typename RetT, typename... Args>
ObjCode<RetT, Args...> CodeGen::compile(IRModule<RetT, Args...> &irModule) const {
    // CPU
    std::string cpuStr = "generic";
    std::string featuresStr = "";

    // todo: do something with options
    llvm::TargetOptions options = InitTargetOptionsFromCodeGenFlags();

    llvm::Reloc::Model relocModel = llvm::Reloc::PIC_;
//        auto codeModel = llvm::CodeModel::Default;
//        auto optLevel = llvm::CodeGenOpt::Default;

    llvm::TargetMachine* targetMachine = target->createTargetMachine(
            this->targetName,
            cpuStr,
            featuresStr,
            options,
            relocModel
    );

    if (targetMachine) {
        irModule.m->setDataLayout(targetMachine->createDataLayout());
        irModule.m->setTargetTriple(this->targetName);

        llvm::CodeGenOpt::Level cgOptLvl = targetMachine->getOptLevel();
        if (cgOptLvl != llvm::CodeGenOpt::Level::None) {
            PR_DBG("CodeGenOpt != None: running Passes.");
            runPasses(*irModule.m, targetMachine);
        }

        auto compiler = llvm::orc::SimpleCompiler(*targetMachine);
        // seems, specific ObjectFile type (e.g. ELF32) and endianness are determined from TargetMachine
        // todo: shouldn't there be irModule.m.release()? no.
        object::OwningBinary<object::ObjectFile> objFile = compiler(*irModule.m);

        if (objFile.getBinary()) {
            // todo: test: there, IRModule's mainSymbol should resolvable (i.e. the same) in objFile
            auto undefinedSyms = findUndefinedSymbols(objFile);
            if (undefinedSyms.empty()) {
                return ObjCode<RetT, Args...>(std::move(objFile), irModule.symbol);


            } else {
                // todo: handle undefined symbols error
                // just return meaningful description; this error is easily resolved and can't be solved at runtime
                return ObjCode<RetT, Args...>();
            }

        } else {
            // todo: handle compilation error
            return ObjCode<RetT, Args...>();
        }

    } else {
        // todo: handle TargetMachine creation error
        return ObjCode<RetT, Args...>();
    }
}

// test: linking pack
// test: compilation pack
//  subtests: different targets
//  subtests: llvm fuzzer?
// test: symbols pack

}

