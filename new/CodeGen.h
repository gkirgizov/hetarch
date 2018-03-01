#pragma once


#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <utility>


#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
//#include "llvm/ADT/StringRef.h"

#include "llvm/Linker/Linker.h"
#include "llvm/Object/ObjectFile.h"

// For setting target
//#include "llvm/ADT/Triple.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/CodeGen/CommandFlags.h"
// For passes
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
// For compilation
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/Object/ObjectFile.h"


#include "supportingClasses.h"
//#include "MemoryManager.h"
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


CodeGen::CodeGen(const std::string &targetName)
        : targetName(llvm::Triple::normalize(llvm::StringRef(targetName)))
        , target(initTarget(this->targetName))
        , pm{initPasses()}
{}

const llvm::Target* CodeGen::initTarget(const std::string &targetName) {

    // Initialise targets
    // todo: optimise somehow? (don't init ALL targets - check how expensive it is)
    //  init from a list of allowed architectures

//    InitializeAllTargetInfos();
//    InitializeAllTargetMCs();
//    InitializeAllAsmPrinters();
//    InitializeAllAsmParsers();

    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();

    std::string targetLookupError;
    // todo: make checkable if CodeGen finds target
    auto target = llvm::TargetRegistry::lookupTarget(targetName, targetLookupError);
    if (!target) {
        // todo: handle error: target not found (err message in 'err')
        return nullptr;
    }
    return target;
}

const llvm::PassRegistry* CodeGen::initPasses() {
    // Initialize default, common passes
    llvm::PassRegistry* pr = llvm::PassRegistry::getPassRegistry();
    initializeCore(*pr);
    initializeCodeGen(*pr);
    initializeLoopStrengthReducePass(*pr);
//    initializeLowerIntrinsicsPass(*pr);
//    initializeUnreachableBlockElimPass(*pr);
    initializeScalarOpts(*pr);
    initializeInstCombine(*pr);
//    initializeAnalysis(*pr);
    return pr;
}


std::vector<std::string> CodeGen::findUndefinedSymbols(object::OwningBinary<object::ObjectFile> &objFile) {
    std::vector<std::string> undefinedSyms;
    for (const auto &sym : objFile.getBinary()->symbols()) {

//        std::cerr
//                << "; type: " << sym.getType().get()
//                << "; flags: 0x" << std::hex << sym.getFlags()
//                << "; name: '" << sym.getName().get().str() << "'"
//                << std::endl;

        // we don't care about SF_FormatSpecific (e.g. debug symbols)
        auto sf = sym.getFlags();
        if (!(sf & llvm::object::BasicSymbolRef::SF_FormatSpecific)
            && sf & llvm::object::BasicSymbolRef::SF_Undefined)
        {
            if (auto name = sym.getName()) {
                undefinedSyms.push_back(name.get().str());
            }
        }
    }
    return undefinedSyms;
}


/*
template<typename IRUnitT>
class PassProvider : public llvm::PassRegistrationListener {
    llvm::PassManager<IRUnitT>& pm;

    void passEnumerate(const llvm::PassInfo* passInfo) override {
        pm.addPass(passInfo->createPass());
    }

public:
    explicit PassProvider(llvm::PassManager<IRUnitT>& pm) : pr{pr} {}

    void addAllPasses(llvm::PassRegistry& pr) const {
        pr.enumerateWith(this)
    }

};
*/


void CodeGen::runPasses(llvm::Module& module, llvm::TargetMachine* tm) const {
    llvm::CodeGenOpt::Level cgOptLvl = tm->getOptLevel();  // todo: map to pass builder opt lvl
    auto optLvl = llvm::PassBuilder::OptimizationLevel::O2;
    llvm::PassBuilder builder{tm};

    llvm::FunctionPassManager fpm = builder.buildFunctionSimplificationPipeline(optLvl, utils::is_debug);  // can be run repeatedly
    llvm::FunctionAnalysisManager fam{utils::is_debug};
    for (llvm::Function& fun : module.functions()) {
        llvm::PreservedAnalyses fpreserved = fpm.run(fun, fam);
    }

    llvm::ModuleAnalysisManager mam{utils::is_debug};
//    llvm::ModulePassManager mpm1 = builder.buildModuleSimplificationPipeline(optLvl, utils::is_debug); // can be run repeatedly
//    llvm::ModulePassManager mpm2 = builder.buildModuleOptimizationPipeline(optLvl, utils::is_debug); // run once
    llvm::ModulePassManager mpm3 = builder.buildPerModuleDefaultPipeline(optLvl, utils::is_debug); // suitable default

    llvm::PreservedAnalyses mpreserved = mpm3.run(module, mam);
}


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

// todo: traverse the dependent modules in __some sane order__ for linking
//  generally, it is DAG, but it should be checked (look for cycles while traversing)
// returns false on error
bool CodeGen::linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies) {
    for (IIRModule &src : dependencies) {
        // currently - DFS without checks for loops
        // note: Passing OverrideSymbols (in flags) as true will have symbols from Src shadow those in the Dest.
        if (!linker.linkInModule(std::move(src.m))) {
            if (CodeGen::linkInModules(linker, src.m_dependencies)) {
                continue;
            };
        }
        // todo: handle link errors (if possible), propagate otherwise
        return false;
    }
    return true;
}

bool CodeGen::link(IIRModule &dependent) {
    llvm::Linker linker(*dependent.m);

    return CodeGen::linkInModules(linker, dependent.m_dependencies);
}

bool CodeGen::link(IIRModule &dest, std::vector<IIRModule> &sources) {
    llvm::Linker linker(*dest.m);

    if (bool isSuccess = CodeGen::linkInModules(linker, dest.m_dependencies)) {
        return CodeGen::linkInModules(linker, sources);
    } else {
        return isSuccess;
    }
}


}


