#pragma once


#include "../conn/IConnection.h"
#include "../MemResident.h"
#include "dsl_type_traits.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


// todo: make Named?
template<typename AddrT, typename T, bool is_const = std::is_const_v<T>>
class ResidentGlobal : public MemResident<AddrT> {
    using value_type = std::remove_reference_t<T>;

    conn::IConnection<AddrT>& conn;
public:
    const AddrT addr;

    ResidentGlobal(
            conn::IConnection<AddrT>& conn,
            mem::MemManager<AddrT>& memManager, mem::MemRegion<AddrT> memRegion, bool unloadable
    )
            : MemResident<AddrT>{memManager, memRegion, unloadable}
            , conn{conn}
            , addr{memRegion.start}
    {}

    // todo: better make static_assert for sane error msg at compile error
    // todo: or allow writing with warning (because is_const is for in-DSL interface, i.e. for type safety)
    inline std::enable_if_t<!is_const> write(const value_type& val) {
        conn.write(addr, sizeof(value_type), utils::toBytes(val));
    };

    value_type read() {
        unsigned char bytes[sizeof(value_type)];
        conn.read(addr, sizeof(value_type), bytes);
        return utils::fromBytes<value_type>(bytes); // copy-construct from reference
    }
};


}
}
