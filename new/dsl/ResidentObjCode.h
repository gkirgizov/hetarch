#pragma once


#include "../MemResident.h"
#include "IDSLCallable.h"


namespace hetarch {
namespace dsl {


template<typename AddrT, typename RetT, typename ...Args> class ResidentObjCode;


template<typename AddrT, typename RetT, typename... Args>
class ECallLoaded: public ECallEmptyBase<RetT, Args...> {
    const ResidentObjCode<AddrT, RetT, Args...> &callee;
    expr_tuple<Args...> args;
public:
    explicit constexpr ECallLoaded(const ResidentObjCode<AddrT, RetT, Args...> &callee, const Expr<Args>&... args)
            : callee{callee}, args{args...} {}

    friend class IRTranslator;
    inline void toIR(IRTranslator &irTranslator) const override { toIRImpl(*this, irTranslator); }
};


template<typename AddrT, typename RetT, typename... Args>
class ResidentObjCode : public MemResident<AddrT>, public dsl::IDSLCallable<RetT, Args...> {
public:
    const AddrT callAddr;

    ResidentObjCode(mem::MemManager<AddrT> *memManager, mem::MemRegion<AddrT> memRegion,
                    AddrT callAddr, bool unloadable)
            : MemResident<AddrT>(memManager, memRegion, unloadable), callAddr(callAddr)
    {}


    inline auto operator()(const Expr<Args>&... args) const { return call(args...); };
    inline auto call(const Expr<Args>&... args) const {
        return ECallLoaded<AddrT, RetT, Args...>(*this, args...);
    }

};


}
}
