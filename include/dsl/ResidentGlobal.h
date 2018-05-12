#pragma once

#include "../conn/IConnection.h"
#include "MemResident.h"
#include "dsl_type_traits.h"
#include "utils.h"


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

    inline void write(const value_type& val) {
        // todo: or allow writing with warning? or remove is_const altogether? --better don't.
        //  (because is_const is for in-DSL interface, i.e. for type safety)
        //  in this case don't inline it with initial_value in IRTranslator!
        static_assert(!is_const, "Const values are not writable!");
        unsigned char bytes[sizeof(value_type)];
        utils::toBytes(val, bytes);
        conn.write(addr, sizeof(value_type), bytes);
    };

    value_type read() {
        unsigned char bytes[sizeof(value_type)];
        conn.read(addr, sizeof(value_type), bytes);
        return utils::fromBytes<value_type>(bytes);
    }
};


}
}
