#pragma once


#include "../IConnection.h"
#include "../MemResident.h"
#include "dsl_type_traits.h"
#include "fun.h"


namespace hetarch {
namespace dsl {


using dsl::f_t; // by some reasons CLion can't resolve it automatically.


template<typename AddrT, typename RetT, typename ...Args> class ResidentObjCode;


template<typename AddrT, typename TdRet, typename... ArgExprs>
using ECallLoaded = ECall<ResidentObjCode<AddrT, TdRet, param_for_arg_t<ArgExprs>...>, ArgExprs...>;


template<typename AddrT, typename TdRet, typename... TdArgs>
struct ResidentObjCode : public dsl::DSLCallable<ResidentObjCode<AddrT, TdRet, TdArgs...>, TdRet, TdArgs...>,
                         public MemResident<AddrT> {
    const AddrT callAddr;

    ResidentObjCode(mem::MemManager<AddrT> *memManager, mem::MemRegion<AddrT> memRegion,
                    AddrT callAddr, bool unloadable)
            : MemResident<AddrT>(*memManager, memRegion, unloadable), callAddr(callAddr)
    {}

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


template<typename>
struct is_dsl_loaded_callable : public std::false_type {};
template<typename AddrT, typename TdRet, typename... TdArgs>
struct is_dsl_loaded_callable<ResidentObjCode<AddrT, TdRet, TdArgs...>> : public std::true_type {};
template<typename T>
inline constexpr bool is_dsl_loaded_callable_v = is_dsl_loaded_callable<T>::value;


template<typename AddrT, typename Td
        , typename = typename std::enable_if_t<is_val_v<Td>> // just to be sure
>
class ResidentGlobal : public MemResident<AddrT>, public Td {
//    using MemResident<AddrT>::MemResident;
//    using simple_type = i_t<Td>;

    conn::IConnection<AddrT>& conn;
public:
    const AddrT addr;

    using type = f_t<Td>;
    using simple_type = remove_cvref_t<type>;

    ResidentGlobal(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memManager, mem::MemRegion<AddrT> memRegion, bool unloadable
    )
            : MemResident<AddrT>{memManager, memRegion, unloadable}
            , conn{conn}
            , addr{memRegion.start}
    {}

    inline std::enable_if_t<!Td::const_q> write(const simple_type& val) {
        conn.write(addr, sizeof(simple_type), reinterpret_cast<char*>(val));
    };

    type read() {
        char bytes[sizeof(simple_type)];
        conn.read(addr, sizeof(simple_type), bytes);
        auto val_ptr = reinterpret_cast<type*>(bytes);
        return *val_ptr;
    }

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};




}
}
