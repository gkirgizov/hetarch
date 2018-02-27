#pragma once


#include <type_traits>

#include "dsl/dsl_type_traits.h"
#include "dsl/dsl_base.h"
#include "dsl/dsl_meta.h"
#include "dsl/ResidentObjCode.h"
#include "CodeLoader.h"
#include "IConnection.h"
#include "MemoryManager.h"


namespace hetarch {


template<typename AddrT>
class Executor {
    conn::IConnection<AddrT>& conn;
    mem::MemManager<AddrT>& memManager;
    dsl::IRTranslator& irt;
    CodeGen& codeGen;

    template<typename T>
    auto load_single_arg(T&& arg, bool write = true) {
//        return CodeLoader::load(conn, memManager, mem::MemType::ReadWrite, arg, write);
        return CodeLoader::load(conn, memManager, mem::MemType::ReadWrite, std::forward<T>(arg), write);
    }

public:
    Executor(conn::IConnection<AddrT>& conn,
             mem::MemManager<AddrT>& memManager,
             dsl::IRTranslator& irt,
             CodeGen& codeGen
    )
            : conn{conn}
            , memManager{memManager}
            , irt{irt}
            , codeGen{codeGen}
    {}

    using SimpleResident = dsl::ResidentObjCode<AddrT, dsl::VoidExpr>;
//    template<>
//    void call<dsl::VoidExpr>(const SimpleResident& f) {
    void call(const SimpleResident& f) {
        if (!conn.call(f.callAddr)) {
            // todo: handle call error
        }
    }

    template<typename TdRet, typename ...TdArgs, typename ...Args>
    auto call(const dsl::ResidentObjCode<AddrT, TdRet, TdArgs...>& f, Args&&... args) {
        // todo: check more formally; like in DSLCallable::call
        using fun_t = dsl::ResidentObjCode<AddrT, TdRet, TdArgs...>;
        using params_t = typename fun_t::args_t;
        using args_t = std::tuple< std::remove_reference_t<Args>... >;
        static_assert(std::is_same_v<params_t, args_t>, "Incorrect arguments!");

        // for each arg
        //  create corresponding const DSLGlobal and load them
        //      ...but why indirection? why not just load raw values?
        //              ah, i see... it is not in-dsl call. it is 'usual' C++ call.

        // Create DSLGlobal for return value and load it
        dsl::DSLGlobal<TdRet> ret_g{};
        dsl::ResidentGlobal ret = CodeLoader::load(conn, memManager, mem::MemType::ReadWrite, ret_g);
        // todo: load all args in one pass
        auto args_g = std::tuple{load_single_arg(args)...};

        // Create dummy function without parameters and return value
        dsl::DSLFunction dummy_caller{
                "dummy_caller",
                {},
                (ret = std::apply(f, args_g), dsl::Unit)
//                (ret = f(load_single_arg(args)...), dsl::Unit)
        };

//        auto dummy_caller = [&](){
//            return (ret = f(load_single_arg(args)...), dsl::Unit)
//        };

        // Translate, compile, load and call it
        IRModule translated = irt.translate(dummy_caller);

        bool verified_ok = utils::verify_module(translated);
        translated.get().dump();
        std::cerr << "verified : " << dummy_caller.name() << ": " << std::boolalpha << verified_ok << std::endl;

        ObjCode compiled = codeGen.compile(translated);
        dsl::ResidentObjCode loaded = CodeLoader::load(conn, memManager, mem::MemType::ReadWrite, compiled);
        call(loaded);
//
//        call(
//                CodeLoader::load(
//                        conn, memManager, mem::MemType::ReadWrite,
//                        codeGen.compile( irt.translate(dummy_caller) )
//                )
//        );

        // todo: unload arguments; unload arguments

        return ret.read();
    }



};



}