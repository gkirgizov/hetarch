#pragma once


#include "../MemResident.h"
#include "fun.h"
//#include "IDSLCallable.h"


namespace hetarch {
namespace dsl {


template<typename AddrT, typename RetT, typename ...Args> class ResidentObjCode;


template<typename AddrT, typename RetT, typename... ArgExprs>
using ECallLoaded = ECall<ResidentObjCode<AddrT, RetT, i_t<ArgExprs>...>, ArgExprs...>;

/*
template<typename AddrT, typename RetT, typename... ArgExprs>
struct ECallLoaded: public ECallEmptyBase<RetT, i_t<ArgExprs>...> {
    using callee_t = ResidentObjCode<AddrT, RetT, i_t<ArgExprs>...>;

    const callee_t& callee;
    std::tuple<const ArgExprs...> args;

    explicit constexpr ECallLoaded(const callee_t& callee, ArgExprs&&... args)
            : callee{callee}, args{std::forward<ArgExprs>(args)...} {}

    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};
*/


template<typename AddrT, typename RetT, typename... Args>
struct ResidentObjCode : public MemResident<AddrT>, public dsl::DSLCallable<ResidentObjCode<AddrT, RetT, Args...>> {
    using ret_t = RetT;
    using args_t = std::tuple<Args...>;

    const AddrT callAddr;

    ResidentObjCode(mem::MemManager<AddrT> *memManager, mem::MemRegion<AddrT> memRegion,
                    AddrT callAddr, bool unloadable)
            : MemResident<AddrT>(memManager, memRegion, unloadable), callAddr(callAddr)
    {}
};


}
}
