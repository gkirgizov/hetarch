#pragma once


#include "IConnection.h
#include "MemoryManager.h"
#include "MemResident.h"

#include "CodeGen.h"
#include "IDSLCallable.h
#include "supportingClasses.h"


namespace hetarch {

template<typename AddrT, typename RetT, typename... Args>
class ResidentObjCode : public MemResident, public IDSLCallable {

public:

    ResidentObjCode(mem::MemManager<AddrT> *memManager, mem::MemRegion<AddrT> memRegion, bool unloadable, AddrT addr)
            : MemResident<AddrT>(memManager, memRegion, unloadable)
    {}

    IDSLVariable<RetT> call(IDSLVariable<Args...>... args) override;

};


// temporary here
class CodeLoader {
public:

    // todo: need IConnection there in interface
    template<typename AddrT, typename RetT, typename... Args>
    static ResidentObjCode<RetT, Args...> load(mem::MemManager<AddrT> *memManager,
                                               const ObjCode<RetT, Args...> &objCode) {
        // if not PIC then resolve symbols
        // strip ObjCode from all extra and find its size
        // alloc memory OF NEEDED TYPE
        //      todo: how to determine what memType we need? always ask user?
        // find displacement of Main Symbol (from the start of memRegion)
        //      or maybe its final address

        // create unloadable resident
        // send
        // return resident
    };

    template<typename AddrT, typename RetT, typename... Args>
    static bool checkEquality(conn::IConnection *c, mem::MemRegion<AddrT> memRegion,
                              const ObjCode<RetT, Args...> &objCode);
};



// example function: avoid all temp. objects (IRModule, ObjCode) if they're not needed
// todo: this ex. shows that all funcs must have && counterparts \
// todo: and all util objects must obey move semantics (not clear about copy)
template<typename RetT, typename... Args>
ResidentObjCode<RetT, Args...> convenientLoad(const DSLFunction<RetT, Args...> &target) {

    IRModule<RetT, Args...> irModule = CodeGen::generateIR(target);
    if (bool success = CodeGen::link(irModule)) {
        return CodeLoader::load(CodeGen::compile(irModule));
    } else {
        return nullptr;
    }
};

}
