#pragma once


#include <string>
#include <string_view>
#include <vector>
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

#include "supportingClasses.h"
#include "dsl/dsl_base.h"
#include "dsl/fun.h"
#include "../tests/test_utils.h"


namespace hetarch {


class CodeGen {
    using llvm_obj_t = llvm::object::OwningBinary<llvm::object::ObjectFile>;

    const std::string targetName;

    const llvm::Target* target{nullptr};
    const llvm::PassRegistry* pm{nullptr};

public:
    explicit CodeGen(const std::string &targetName);

    template<typename RetT, typename... Args>
    inline ObjCode<RetT, Args...> compile(IRModule<RetT, Args...> &irModule) const {
        return ObjCode<RetT, Args...>(compileImpl(irModule), irModule.symbol);
    };

    static bool link(IIRModule &dependent);

    static bool link(IIRModule &dest, std::vector<IIRModule> &sources);

    CodeGen(const CodeGen &) = delete;
    CodeGen &operator=(const CodeGen &) = delete;

private:
    llvm_obj_t compileImpl(IIRModule& irModule) const;

    static bool linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies);

    static std::vector<std::string> findUndefinedSymbols(llvm_obj_t& objFile);

    static const llvm::Target* initTarget(const std::string& targetName);
    static const llvm::PassRegistry* initPasses();

    void runPasses(llvm::Module& module, llvm::TargetMachine* tm = nullptr) const;
};


// test: linking pack
// test: compilation pack
//  subtests: different targets
//  subtests: llvm fuzzer?
// test: symbols pack

}

