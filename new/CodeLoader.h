#pragma once


#include "IConnection.h"
#include "MemoryManager.h"
#include "MemResident.h"

#include "CodeGen.h"
#include "dsl/IDSLCallable.h"
#include "dsl/ResidentObjCode.h"
#include "supportingClasses.h"

#include "../tests/utils.h"


namespace hetarch {

// temporary here
class CodeLoader {
    using content_t = llvm::StringRef;

public:
    template<typename AddrT, typename RetT, typename... Args>
    static dsl::ResidentObjCode<AddrT, RetT, Args...> load(conn::IConnection<AddrT> *conn,
                                               mem::MemManager<AddrT> *memManager,
                                               mem::MemType memType,
                                               const ObjCode<RetT, Args...> &objCode) {
        // if not PIC then relocate symbols ??? but it must be done after allocated memManager...

        // strip ObjCode from all extra things
        auto contents = getContents(objCode);
        // find its size
        auto contentsSize = reinterpret_cast<AddrT>(contents.size());

        // alloc memory of needed type
        auto memRegion = memManager->alloc(contentsSize, memType);
        if (memRegion.size >= contentsSize) {

            // send contents through IConnection
            conn->write(memRegion.start, contentsSize, contents.data());

            // log for debug
            utils::dumpSections(*objCode.getBinary());
            utils::dumpSymbols(*objCode.getBinary());

            // todo: load only '.text' section. otherwise need to implement relocations and all the stuff here
            // get offset of needed symbol
            if (auto sectionOffset = objCode.getSymbol().getAddress()) {
                auto addr = sectionOffset.get();
                // create unloadable resident
                return dsl::ResidentObjCode<AddrT, RetT, Args...>(memManager, memRegion, addr, true);

            } else {
                // todo: handle some curious thing with unknown address (offset???) of the symbol
            }
        } else {
            // todo: throw mem error
        }
    };

    template<typename AddrT, typename RetT, typename... Args>
    static dsl::ResidentObjCode<AddrT, RetT, Args...> load(conn::IConnection<AddrT> *conn,
                                               mem::MemRegion<AddrT> memRegion,
                                               const ObjCode<RetT, Args...> &objCode) {
        //

        // call private_load
    };

    template<typename AddrT, typename RetT, typename... Args>
    static bool checkEquality(conn::IConnection<AddrT> *conn,
                              mem::MemRegion<AddrT> memRegion,
                              const ObjCode<RetT, Args...> &objCode);

private:
    template<typename RetT, typename... Args>
    static content_t getContents(const ObjCode<RetT, Args...> &objCode) {
        // strip ObjCode from all extra things and
        // return actual contents
        return objCode.getBinary()->getData();
    };

};



// example function: avoid all temp. objects (IRModule, ObjCode) if they're not needed
// todo: this ex. shows that all funcs must have && counterparts \
// todo: and all util objects must obey move semantics (not clear about copy)
//template<typename RetT, typename... Args>
//dsl::ResidentObjCode<RetT, Args...> convenientLoad(const DSLFunction<RetT, Args...> &target) {
//
//    IRModule<RetT, Args...> irModule = CodeGen::generateIR(target);
//    if (bool success = CodeGen::link(irModule)) {
//        return CodeLoader::load(CodeGen::compile(irModule));
//    } else {
//        return nullptr;
//    }
//};


}
