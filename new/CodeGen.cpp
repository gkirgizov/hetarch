#include "CodeGen.h"


namespace hetarch {


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
