#pragma once


#include "IConnection.h"
#include "MemoryManager.h"
#include "MemResident.h"
#include "CodeGen.h"

#include "dsl/dsl_type_traits.h"
#include "dsl/IRTranslator.h"
#include "supportingClasses.h"
#include "dsl/ResidentObjCode.h"
#include "dsl/fun.h"

#include "../tests/test_utils.h"


namespace hetarch {

class CodeLoader {
    using content_t = llvm::StringRef;

public:

    /*    template<typename AddrT, typename Td>
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
            utils::printSectionName(section);

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
    }*/

    template<typename AddrT, typename Td>
    static auto load(conn::IConnection<AddrT> &conn,
                     mem::MemManager<AddrT> &memManager,
                     mem::MemType memType,
                     const dsl::DSLGlobal<Td>& g)
    {
        return load(conn, memManager, memType, g.x);
    };

    template<typename AddrT, typename T, bool is_const, bool is_volatile>
    static auto load(conn::IConnection<AddrT> &conn,
                     mem::MemManager<AddrT> &memManager,
                     mem::MemType memType,
                     const dsl::Var<T, is_const, is_volatile>& g)
    {
        using val_type = T;
        using Td = dsl::Var<val_type, is_const, is_volatile>;

        const auto size = reinterpret_cast<AddrT>(sizeof(val_type));
        auto memRegion = memManager.alloc(size, memType);
        if (memRegion.size >= size) {

            if (g.initialised()) {
                // todo: endianness?
                conn.write(memRegion.start, size, toBytes(g.initial_val()));
            }

            // todo: is it always unloadable
            return dsl::ResidentVar<AddrT, T, is_const, is_volatile>{
                    conn, memManager, memRegion, true,
                    g.initial_val(), g.name()
            };

        } else {
            // todo: handle not enough mem
        }

    };


    template<typename AddrT, typename RetT, typename... Args>
    static dsl::ResidentObjCode<AddrT, RetT, Args...> load(conn::IConnection<AddrT>& conn,
                                               mem::MemManager<AddrT>& memManager,
                                               mem::MemType memType,
                                               const ObjCode<RetT, Args...>& objCode) {
        // Load only '.text' section. otherwise need to implement relocations and all the stuff here

        // LOG FOR DEBUG
        utils::dumpSections(*objCode.getBinary());
        utils::dumpSymbols(*objCode.getBinary());

        const llvm::object::SymbolRef& symbol = objCode.getSymbol();
//        for (const auto& section_exp : symbol.getSection()) {
        if(auto it_exp = symbol.getSection()) {
            const llvm::object::SectionRef& section = *it_exp.get();
            const auto contentsSize = reinterpret_cast<AddrT>(section.getSize());

            assert(section.isText());

            llvm::StringRef contents;
            auto err_code = section.getContents(contents);
            if (!err_code) {

                // LOG FOR DEBUG
                std::cerr << "contentsSize=" << contentsSize << "; contents.size()=" << contents.size() << std::endl;
                assert(contents.size() == contentsSize);

                auto memRegion = memManager.alloc(contentsSize, memType);
                if (memRegion.size >= contentsSize) {

                    conn.write(memRegion.start, contentsSize, contents.data());

                    // get offset of needed symbol
                    if (auto offsetInSection = symbol.getAddress()) {
                        const auto offset = reinterpret_cast<AddrT>(offsetInSection.get());
                        const AddrT callAddr = offset + memRegion.start;
                        // Create unloadable resident
                        return dsl::ResidentObjCode<AddrT, RetT, Args...>(memManager, memRegion, callAddr, true);

                    } else {
                        // todo: handle some curious thing with unknown address (offset???) of the symbol
                    }
                } else {
                    // todo: throw mem error
                }
            } else {
                // todo: handle err_code
            }
        } else {
            // todo: handle section error
        }
    };


    template<typename AddrT, typename RetT, typename... Args>
    static bool checkEquality(conn::IConnection<AddrT>& conn,
                              mem::MemRegion<AddrT> memRegion,
                              const ObjCode<RetT, Args...>& objCode);

    // todo: specialise translating initial value to char* (i.e. for Array, for Struct)
    template<typename T>
    static const char* toBytes(const T &x) {
        if constexpr (std::is_arithmetic_v<T>) {
            return reinterpret_cast<const char*>(&x);
        }
    }
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
