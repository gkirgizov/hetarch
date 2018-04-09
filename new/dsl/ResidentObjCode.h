#pragma once

#include <string>
#include <array>
#include <tuple>

#include "../conn/IConnection.h"
#include "../MemResident.h"

#include "dsl_type_traits.h"
#include "fun.h"


namespace hetarch {

template<typename AddrT> class Executor;

namespace dsl {


using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename AddrT, typename TdRet, typename... TdArgs>
class ResidentObjCode : public dsl::Callable<ResidentObjCode<AddrT, TdRet, TdArgs...>, TdRet, TdArgs...>
                      , public MemResident<AddrT>
                      , public Named
{
    friend class Executor<AddrT>;

    using ret_space_t = TdRet;
    using args_space_t = std::tuple< TdArgs... >;

    ret_space_t remoteRet;
    args_space_t remoteArgs;
    conn::IConnection<AddrT>& conn;

public:
    const AddrT callAddr;

    ResidentObjCode(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memManager, mem::MemRegion<AddrT> memRegion,
            AddrT callAddr, bool unloadable,
            args_space_t args, ret_space_t ret,
            const std::string& name = ""
    )
            : MemResident<AddrT>(memManager, memRegion, unloadable)
            , Named{name}
            , callAddr(callAddr)
            , conn{conn}
            , remoteRet{ret}
            , remoteArgs{args}
    {}

    auto getCaller() const {
//        const std::string fun_name = std::string{"caller_for_"} + std::string(name());
//        const std::string fun_name = std::string{"dummy_caller_for_"};
        if constexpr (!is_unit_v<TdRet>) {
            return dsl::Function{
//                    fun_name,
                    "dummy_caller",
                    {},
                    (remoteRet = std::apply(*this, remoteArgs), dsl::Unit)
            };
        } else {
            return dsl::Function{
//                    fun_name,
                    "dummy_caller",
                    {},
                    (std::apply(*this, remoteArgs), dsl::Unit)
            };
        }
    }

    void unload() override {
        if constexpr (!is_unit_v<TdRet>) {
            remoteRet.unload();
        }
        // for_each in tuple
        std::apply([](auto& ...remotes){ std::make_tuple((
                remotes.unload()
        , 0)... ); }, remoteArgs);
    }

    template<typename ...Args>
    void sendArgs(Args... actualArgs) {
        using Inds = std::make_integer_sequence<AddrT, sizeof...(TdArgs)>;
        sendArgsImpl(Inds{}, actualArgs...);
    }

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }

private:
    template<AddrT ...I, typename ...Args>
    void sendArgsImpl(std::integer_sequence<AddrT, I...>, Args... actualArgs) {
        std::make_tuple( 0, ( std::get<I>(remoteArgs).write(actualArgs) , 0)... );
    }

};


template<typename AddrT, typename TdRet, typename... ArgExprs>
using ECallLoaded = ECall<ResidentObjCode<AddrT, TdRet, param_for_arg_t<ArgExprs>...>, ArgExprs...>;

template<typename>
struct is_dsl_loaded_callable : std::false_type {};
template<typename AddrT, typename TdRet, typename... TdArgs>
struct is_dsl_loaded_callable<ResidentObjCode<AddrT, TdRet, TdArgs...>> : std::true_type {};
template<typename T>
inline constexpr bool is_dsl_loaded_callable_v = is_dsl_loaded_callable<T>::value;


}
}
