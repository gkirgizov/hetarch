#pragma once


#include "../IConnection.h"
#include "../MemResident.h"
#include "dsl_type_traits.h"


namespace hetarch {
namespace dsl {


template<typename AddrT, typename T, bool is_const = std::is_const_v<T>>
class ResidentGlobal : public MemResident<AddrT> {
//    using value_type = i_t<Td>;
    using value_type = std::remove_reference_t<T>;

    conn::IConnection<AddrT>& conn;
public:
    const AddrT addr;

    ResidentGlobal(ResidentGlobal<AddrT, T, is_const>&& ) = default; // todo: wtf
//    ResidentGlobal(ResidentGlobal<AddrT, T, is_const>&& other)
//            : MemResident<AddrT>(static_cast< MemResident<AddrT>&& >(std::move(other)))
//            , conn{other.conn}
//            , addr{other.addr}
//    {}

    ResidentGlobal(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memManager, mem::MemRegion<AddrT> memRegion, bool unloadable
    )
            : MemResident<AddrT>{memManager, memRegion, unloadable}
            , conn{conn}
            , addr{memRegion.start}
    {}

    // todo: better make static_assert for sane error msg at compile error
    inline std::enable_if_t<!is_const> write(const value_type& val) {
        conn.write(addr, sizeof(value_type), reinterpret_cast<char*>(val));
    };

    value_type read() {
        char bytes[sizeof(value_type)];
        conn.read(addr, sizeof(value_type), bytes);
        auto val_ptr = reinterpret_cast<value_type*>(bytes);
        // todo: bytes go out of scope; dangling ref? no, value_type copy-ctred
        return *val_ptr;
    }

    inline void toIR(IRTranslator &irTranslator) const { toIRImpl(*this, irTranslator); }
};


}
}