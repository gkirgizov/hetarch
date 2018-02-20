#pragma once


#include "../MemResident.h"
#include "dsl_type_traits.h"
#include "fun.h"


namespace hetarch {
namespace dsl {


template<typename AddrT, typename RetT, typename ...Args> class ResidentObjCode;


template<typename AddrT, typename TdRet, typename... ArgExprs>
using ECallLoaded = ECall<ResidentObjCode<AddrT, TdRet, f_t<ArgExprs>...>, ArgExprs...>;


template<typename AddrT, typename RetT, typename... Args>
struct ResidentObjCode : public MemResident<AddrT>, public dsl::DSLCallable<ResidentObjCode<AddrT, RetT, Args...>, Args...> {
    const AddrT callAddr;

    ResidentObjCode(mem::MemManager<AddrT> *memManager, mem::MemRegion<AddrT> memRegion,
                    AddrT callAddr, bool unloadable)
            : MemResident<AddrT>(memManager, memRegion, unloadable), callAddr(callAddr)
    {}
};


template<typename>
struct is_dsl_loaded_callable : public std::false_type {};
template<typename AddrT, typename TdRet, typename... TdArgs>
struct is_dsl_loaded_callable<ResidentObjCode<AddrT, TdRet, TdArgs...>> : public std::true_type {};
template<typename T>
inline constexpr bool is_dsl_loaded_callable_v = is_dsl_loaded_callable<T>::value;


template<typename AddrT, typename Td>
struct ResidentGlobal : public MemResident<AddrT> {
    using MemResident<AddrT>::MemResident;

    const AddrT addr;

    ResidentGlobal(mem::MemManager<AddrT> *memManager, mem::MemRegion<AddrT> memRegion, bool unloadable)
            : MemResident<AddrT>{memManager, memRegion, unloadable}, addr{memRegion.start} {}

};


}
}
