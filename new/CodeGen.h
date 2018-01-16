#pragma once


#include <string>
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
#include "llvm/IR/PassManager.h"

// For compilation
#include "llvm/ExecutionEngine/Orc/CompileUtils.h"
#include "llvm/Object/ObjectFile.h"


#include "supportingClasses.h"
//#include "MemoryManager.h"
#include "dsl/IDSLCallable.h"


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
    ObjCode<RetT, Args...> compile(IRModule<RetT, Args...> &irModule);


    template<typename RetT, typename... Args>
    static IRModule<RetT, Args...> generateIR(const dsl::DSLFunction<RetT, Args...> &f);


    static bool link(IIRModule &dependent);

    static bool link(IIRModule &dest, std::vector<IIRModule> &sources);

    CodeGen(const CodeGen &) = delete;
    CodeGen &operator=(const CodeGen &) = delete;

private:
    static bool linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies);

    static std::vector<std::string> findUndefinedSymbols(object::OwningBinary<object::ObjectFile> &objFile);

};

CodeGen::CodeGen(const std::string &targetName)
: targetName(llvm::Triple::normalize(llvm::StringRef(targetName)))
//, target(std::unique_ptr(llvm::TargetRegistry::lookupTarget(this->targetName, targetLookupError)))
{
    // Initialise targets
    // todo: optimise somehow? (don't init ALL targets - check how expensive it is)
//    InitializeAllTargetInfos();
//    InitializeAllTargetMCs();
//    InitializeAllAsmPrinters();
//    InitializeAllAsmParsers();

    LLVMInitializeX86TargetInfo();
    LLVMInitializeX86Target();
    LLVMInitializeX86TargetMC();
    LLVMInitializeX86AsmPrinter();

    // Initialize default, common passes
//    auto passRegistry = llvm::PassRegistry::getPassRegistry();
//    initializeCore(*passRegistry);
//    initializeCodeGen(*passRegistry);

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
            undefinedSyms.push_back(sym.getName().get().str());
        }
    }
    return undefinedSyms;
}


template<typename RetT, typename... Args>
ObjCode<RetT, Args...> CodeGen::compile(IRModule<RetT, Args...> &irModule) {

    std::string targetLookupError;
    // todo: move at least that to the ctor
    // todo: make checkable if CodeGen finds target
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
                auto passManager = llvm::PassManager<llvm::Module>();
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
        }
    } else {
        // todo: handle error: target not found (err message in 'err')
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



template<typename RetT, typename ...Args>
IRModule<RetT, Args...> CodeGen::generateIR(const dsl::DSLFunction<RetT, Args...> &dslFunction) {
    std::string moduleName{"module"};  // todo: module name
    const char* const funcName = "func"; // todo: function name (i.e. symbol name)

    llvm::LLVMContext context;
    auto module = std::make_unique<llvm::Module>(moduleName, context);

    llvm::Type *void_type = llvm::Type::getVoidTy(context);  // todo: somehow translate C++ types to llvm::Type
    llvm::FunctionType *f_type = llvm::FunctionType::get(void_type, {}, false);
    llvm::Function *fun = llvm::Function::Create(f_type, llvm::Function::ExternalLinkage, funcName, module.get());

    llvm::BasicBlock *entry_bb = llvm::BasicBlock::Create(context, "entry", fun);
    llvm::IRBuilder<> b{entry_bb};
    // that's it. now we have builder and func and module




    // ...
    // ...
    return IRModule<RetT, Args...>{std::move(module), funcName};
}


}


