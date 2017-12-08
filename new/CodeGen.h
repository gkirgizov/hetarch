#pragma once


#include <string>
#include <vector>
#include <memory>


#include "llvm/IR/Module.h"

#include "llvm/Linker/Linker.h"

#include "llvm/Object/ObjectFile.h"


#include "supportingClasses.h
#include "MemoryManager.h"

// temporary, mock
#include "IDSLCallable.h"


namespace hetarch {

class CodeGen {
public:
    template<typename RetT, typename... Args>
    static IRModule<RetT, Args...> generateIR(DSLFunction<RetT, Args...> dslFunction);

    template<typename RetT, typename... Args>
    static bool link(IRModule<RetT, Args...> &dependent);

    template<typename RetT, typename... Args>
    static bool link(IRModule<RetT, Args...> &dest, std::vector<IIRModule> &sources);

    template<typename RetT, typename... Args>
    static ObjCode<RetT, Args...> compile(const IRModule<RetT, Args...> &irModule);

    CodeGen() = delete;
    //etc. for other specials

private:
    static bool linkInModules(llvm::Linker &linker, std::vector<IIRModule> &dependencies);


};


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

