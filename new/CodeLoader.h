#pragma once

#include <numeric>

#include "supportingClasses.h"
#include "conn/IConnection.h"
#include "MemoryManager.h"
#include "MemResident.h"
#include "CodeGen.h"

#include "dsl/dsl_type_traits.h"
#include "dsl/fun.h"
#include "dsl/ResidentObjCode.h"
#include "dsl/IRTranslator.h"

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

    template<typename AddrT>
    inline static auto getResident(const dsl::VoidExpr& v,
                                   conn::IConnection<AddrT> &conn,
                                   mem::MemManager<AddrT> &memManager,
                                   mem::MemRegion<AddrT> memRegion,
                                   bool unloadable = true)
    { return v; };

    /// Return ResidentGlobal residing at addr for global g; don't actually load anything.
    /// It is a fabric for ResidentGlobals.
    template<typename AddrT, typename Td>
    inline static auto getLoadedResident(const dsl::DSLGlobal<Td>& g,
                                         AddrT addr,
                                         conn::IConnection<AddrT> &conn,
                                         mem::MemManager<AddrT> &memManager,
                                         mem::MemType memType = mem::MemType::ReadWrite,
                                         bool unloadable = false)

    {
        using val_type = dsl::f_t<Td>;
        const auto size = static_cast<AddrT>(sizeof(val_type));
        mem::MemRegion<AddrT> memRegion{addr, size, memType};
        return getResident(g.x, conn, memManager, memRegion, unloadable);
    };

    /// Try load global g at specified addr.
    template<typename AddrT, typename Td>
    static auto load(const dsl::DSLGlobal<Td>& g,
                     AddrT addr,
                     conn::IConnection<AddrT> &conn,
                     mem::MemManager<AddrT> &memManager,
                     mem::MemType memType = mem::MemType::ReadWrite)
    {
        using val_type = dsl::f_t<Td>;

        const auto size = static_cast<AddrT>(sizeof(val_type));
        mem::MemRegion<AddrT> memRegion{addr, size, memType};
        auto actualMemRegion = memManager.tryAlloc(memRegion);
        if (actualMemRegion.size >= size) {
            if (g.x.initialised()) {
                // todo: endianness?
                conn.write(actualMemRegion.start, size, utils::toBytes(g.x.initial_val()));
            }
            return getResident(g.x, conn, memManager, actualMemRegion, true);

        } else {
            // todo: handle tryAlloc errors
        }
    };


    /// Load global g at some addr.
    template<typename AddrT, typename Td>
    static auto load(const dsl::DSLGlobal<Td>& g,
                     conn::IConnection<AddrT> &conn,
                     mem::MemManager<AddrT> &memManager,
                     mem::MemType memType = mem::MemType::ReadWrite)

    {
        using val_type = dsl::f_t<Td>;

        const auto size = static_cast<AddrT>(sizeof(val_type));
        auto memRegion = memManager.alloc(size, memType);
        if (memRegion.size >= size) {
            if (g.x.initialised()) {
                // todo: endianness?
                conn.write(memRegion.start, size, utils::toBytes(g.x.initial_val()));
            }
            return getResident(g.x, conn, memManager, memRegion, true);

        } else {
            // todo: handle not enough mem
        }
    };

    template<typename AddrT, typename T, bool is_const, bool is_volatile>
    inline static auto getResident(const dsl::Var<T, is_const, is_volatile>& g,
                                   conn::IConnection<AddrT> &conn,
                                   mem::MemManager<AddrT> &memManager,
                                   mem::MemRegion<AddrT> memRegion,
                                   bool unloadable = true)
    {
        // todo: what about is_const for func params? it is an error (not being able to .write() to it)
        return dsl::ResidentVar<AddrT, T, false, is_volatile>{
                conn, memManager, memRegion, unloadable,
                g.initial_val(), g.name()
        };
    };

    template<typename AddrT, typename TdElem, std::size_t N, bool is_const>
    inline static auto getResident(const dsl::Array<TdElem, N, is_const>& g,
                                   conn::IConnection<AddrT> &conn,
                                   mem::MemManager<AddrT> &memManager,
                                   mem::MemRegion<AddrT> memRegion,
                                   bool unloadable = true)
    {
        return dsl::ResidentArray<AddrT, TdElem, N, false>{
                conn, memManager, memRegion, unloadable,
                g.initial_val(), g.name()
        };
    };

    template<typename AddrT, typename Td, bool is_const>
    inline static auto getResident(const dsl::RawPtr<Td, is_const>& g,
                                   conn::IConnection<AddrT> &conn,
                                   mem::MemManager<AddrT> &memManager,
                                   mem::MemRegion<AddrT> memRegion,
                                   bool unloadable = true)
    {
        return dsl::ResidentPtr<AddrT, Td, false>{
                conn, memManager, memRegion, unloadable,
                g.initial_val(), g.name()
        };
    }

    template<typename AddrT, typename TdRet, typename... TdArgs>
    static auto load(conn::IConnection<AddrT>& conn,
                     mem::MemManager<AddrT>& memManager,
                     mem::MemType memType,
                     dsl::IRTranslator& irt,
                     CodeGen& codeGen,
                     const ObjCode<TdRet, TdArgs...>& objCode)
    {
        // load only '.text' section. otherwise need to implement relocations and all the stuff here

        if constexpr (utils::is_debug) {
            std::cerr << "DUMPING: " << objCode.symbol << std::endl;
            utils::dumpSections(*objCode.getBinary());
            utils::dumpSymbols(*objCode.getBinary());
        }

        const llvm::object::SymbolRef& symbol = objCode.getSymbol();
//        for (const auto& section_exp : symbol.getSection()) {
        if(auto it_exp = symbol.getSection()) {
            const llvm::object::SectionRef& section = *it_exp.get();
            const auto contentsSize = static_cast<AddrT>(section.getSize());

            assert(section.isText());

            llvm::StringRef contents;
            auto err_code = section.getContents(contents);
            if (!err_code) {

                if constexpr (utils::is_debug) {
                    std::cerr << "contentsSize=" << contentsSize << "; contents.size()=" << contents.size() << std::endl;
                    assert(contents.size() == contentsSize);
                }

                // Allocate all mem we need
                auto memForText = memManager.alloc(contentsSize, memType);

                // Allocate memory for return value
                AddrT retValSize = 0;
                using val_type = dsl::f_t<TdRet>;
                if constexpr (!std::is_void_v<val_type>) {
                    retValSize = sizeof(val_type);
                }
                auto memForRetVal = memManager.alloc(retValSize, memType);

                // Allocate memory for arguments
                std::array<AddrT, sizeof...(TdArgs)> argsSizes{ sizeof(dsl::f_t<TdArgs>)... };
                const AddrT argsSize = ( sizeof(dsl::f_t<TdArgs>) + ... + 0 );
                auto memForArgs = memManager.allocMany(argsSizes, memType);
                AddrT allocatedSize = 0;
                for (const auto& mr : memForArgs) { allocatedSize += mr.size; }

                if (memForText.size >= contentsSize && allocatedSize >= argsSize && memForRetVal.size >= retValSize) {
                    // get offset of needed symbol
                    if (auto offsetInSection = symbol.getAddress()) {
                        const auto offset = static_cast<AddrT>(offsetInSection.get());
                        const AddrT callAddr = offset + memForText.start;

                        // Load everything we need
                        conn.write(memForText.start, contentsSize, reinterpret_cast<const unsigned char*>(contents.data()));
                        static_assert((std::is_default_constructible_v<TdRet> && ... && std::is_default_constructible_v<TdArgs>));
                        auto retLoaded = getResident(TdRet{}, conn, memManager, memForRetVal, true);
                        // Load arguments (use apply to unpack memForArgs)
                        auto argsLoaded = std::apply([&](auto... memForArg){
                            return std::tuple{ getResident(TdArgs{}, conn, memManager, memForArg, true)... };
                        }, memForArgs);

                        // Create unloadable resident
//                        return dsl::ResidentObjCode<AddrT, TdRet, TdResArgs...>(
                        return dsl::ResidentObjCode(
                                conn,
                                memManager, memForText,
                                callAddr, true,
                                std::move(argsLoaded),
                                std::move(retLoaded),
                                objCode.symbol
                        );

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


}
