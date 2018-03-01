#pragma once


#include <type_traits>
#include <tuple>

#include "CodeLoader.h"
#include "IConnection.h"
#include "MemoryManager.h"
#include "dsl/dsl_type_traits.h"
#include "dsl/dsl_base.h"
#include "dsl/dsl_meta.h"
#include "dsl/ResidentObjCode.h"
#include "dsl/IRTranslator.h"
#include "../tests/test_utils.h"


namespace hetarch {


template<typename AddrT>
class Executor {
    conn::IConnection<AddrT>& conn;
    mem::MemManager<AddrT>& memManager;
    dsl::IRTranslator& irt;
    CodeGen& codeGen;

    template<AddrT ...I, typename ...TdArgs, typename ...Args>
    auto sendArgs(std::integer_sequence<AddrT, I...>,
                  std::tuple<TdArgs...>& args,
                  Args&&... actualArgs)
    {
        std::tuple dummy__ = { std::get<I>(args).write(actualArgs)... };
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
    void call(SimpleResident& f) {
        if (!conn.call(f.callAddr)) {
            // todo: handle call error
        }
    }

    template<typename TdRet, typename ...TdArgs, typename ...Args>
    auto call(dsl::ResidentObjCode<AddrT, TdRet, TdArgs...>& f, Args&&... args) {
        // todo: check more formally; like in DSLCallable::call
        using fun_t = dsl::ResidentObjCode<AddrT, TdRet, TdArgs...>;
        using params_t = typename fun_t::args_t;
        using args_t = std::tuple< std::remove_reference_t<Args>... >;
        static_assert(std::is_same_v<params_t, args_t>, "Incorrect arguments!");

        // Send arguments
        f.sendArgs(std::forward<Args>(args)...);

        // Translate, compile, load and call it
        dsl::DSLFunction dummy_caller = f.getCaller();
        IRModule translated = irt.translate(dummy_caller);

#ifdef DEBUG
        bool verified_ok = utils::verify_module(translated);
        translated.get().dump();
        std::cerr << "verified : " << dummy_caller.name() << ": " << std::boolalpha << verified_ok << std::endl;
#endif

        ObjCode compiled = codeGen.compile(translated);
        dsl::ResidentObjCode loaded = CodeLoader::load(conn, memManager, mem::MemType::ReadWrite, irt, codeGen, compiled);
        call(loaded);

        PR_DBG("loaded dummy_caller");
//
//        call(
//                CodeLoader::load(
//                        conn, memManager, mem::MemType::ReadWrite,
//                        irt, codeGen,
//                        codeGen.compile( irt.translate(dummy_caller) )
//                )
//        );

        return f.remoteRet.read();
    }


};



}