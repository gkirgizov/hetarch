#pragma once


#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <memory>
#include <utility>

#include "llvm/IR/Module.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Linker/Linker.h"
//#include "llvm/ADT/Triple.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"

#include "supportingClasses.h"
#include "dsl/dsl_base.h"
#include "dsl/fun.h"
#include "../tests/test_utils.h"


namespace hetarch {


class CodeGen {
    using CGOptLvl = llvm::CodeGenOpt::Level;
public:
    using llvm_obj_t = llvm::object::OwningBinary<llvm::object::ObjectFile>;
    using OptLvl = llvm::PassBuilder::OptimizationLevel;

    explicit CodeGen(const std::string &targetName);

    template<typename RetT, typename... Args>
    inline ObjCode<RetT, Args...> compile(IRModule<RetT, Args...> &irModule, OptLvl optLvl = OptLvl::O2) const {
        return ObjCode<RetT, Args...>(compileImpl(irModule, optLvl), irModule.symbol);
    };

    inline void runPasses(IIRModule& module, OptLvl optLvl) const;

    static bool link(IIRModule &dependent);

    static bool link(IIRModule &dest, std::vector<IIRModule> &sources);

    CodeGen(const CodeGen &) = delete;
    CodeGen &operator=(const CodeGen &) = delete;

private:
    const std::string targetName;
    const llvm::Target* target{nullptr};
    const llvm::PassRegistry* pm{nullptr};

    const std::map<OptLvl, CGOptLvl> opt_lvl_map{
            { OptLvl::O0, CGOptLvl::None },
            { OptLvl::O1, CGOptLvl::Less },
            { OptLvl::O2, CGOptLvl::Default },
            { OptLvl::Os, CGOptLvl::Default },
            { OptLvl::O3, CGOptLvl::Aggressive },
            { OptLvl::Oz, CGOptLvl::Less } // is it so?
    };

    llvm::TargetMachine* getTargetMachine(OptLvl optLvl) const;
    llvm_obj_t compileImpl(IIRModule& irModule, OptLvl optLvl) const;
    void runPassesImpl(llvm::Module& module, llvm::TargetMachine* tm = nullptr) const;

    static bool linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies);

    static std::vector<std::string> findUndefinedSymbols(llvm_obj_t& objFile);

    static const llvm::Target* initTarget(const std::string& targetName);
    static const llvm::PassRegistry* initPasses();
};


// test: linking pack
// test: compilation pack
//  subtests: different targets
//  subtests: llvm fuzzer?
// test: symbols pack

}

