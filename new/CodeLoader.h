#pragma once


#include "IConnection.h"
#include "MemoryManager.h"
#include "MemResident.h"
#include "CodeGen.h"

#include "dsl/IRTranslator.h"
#include "supportingClasses.h"
#include "dsl/ResidentObjCode.h"
#include "dsl/fun.h"

#include "../tests/test_utils.h"


namespace hetarch {


class CodeLoader {
    using content_t = llvm::StringRef;

public:
    template<typename AddrT, typename Td>
    static dsl::ResidentGlobal<AddrT, Td> load(conn::IConnection<AddrT>& conn,
                                               mem::MemManager<AddrT> *memManager,
                                               mem::MemType memType,
                                               dsl::IRTranslator& irt,
                                               const CodeGen& cg,
                                               const dsl::DSLGlobal<Td>& g) {

        IRModule<Td> g_module = irt.translate(g);
        std::cerr << "DUMPING g_module:" << std::endl;
        g_module.get().dump();
        std::cerr << "DUMPED g_module." << std::endl;

        ObjCode<Td> g_binary = cg.compile(g_module);

//        auto contents = getContents(g_binary);
        // todo: ?? g_binary.getCommonSymbolSize()

        // log for debug
        utils::dumpSections(*g_binary.getBinary());
        utils::dumpSymbols(*g_binary.getBinary());

        const llvm::object::SymbolRef& symbol = g_binary.getSymbol();
//        for (const auto& section_exp : symbol.getSection()) {
        if(auto it_exp = symbol.getSection()) {
            const llvm::object::SectionRef& section = *it_exp.get();

            // debug things
            StringRef section_name;
            auto err_code0 = section.getName(section_name);
            if (!err_code0) {
                std::cerr << "section_name: " << section_name.str() << std::endl;
            } else {
                std::cerr << "error when accessing section.getName(): " << err_code0 << std::endl;
            }

//            assert(g.x.initialised() && section.isData() || !g.x.initialised() && section.isBSS());
//            assert(section.isData() || section.isBSS());

            llvm::StringRef contents;
            auto err_code = section.getContents(contents);
            if (!err_code) {

                const AddrT contentsSize = section.getSize();
                auto memRegion = memManager->alloc(contentsSize, memType);

                std::cerr << "contentsSize=" << contentsSize << "; contents.size()=" << contents.size() << std::endl;
                assert(contents.size() == contentsSize);

                conn.write(memRegion.start, contentsSize, contents.data());
                return dsl::ResidentGlobal<AddrT, Td>{conn, memManager, memRegion, true};

            } else {
                // todo: handle err_code
            }
        } else {
            // todo: handle section error
        }
    }

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
            if (auto offsetInSection = objCode.getSymbol().getAddress()) {
                auto addr = offsetInSection.get();
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

    template<typename RetT, typename... Args>
    static auto getSection(const ObjCode<RetT, Args...> &objCode, const std::string& sectionName) {
        for (const auto& section : objCode.getBinary()->sections()) {
            llvm::StringRef name;
            std::string id;
            auto err_code = section.getName(name);
            if (!err_code && name.str() == sectionName) {
                return section;
            }
        }
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
