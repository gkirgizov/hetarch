#pragma once

#include <cstdint>
#include <sys/mman.h>

#include "IConnection.h"
#include "ht_proto.h"
#include "../utils.h"


namespace hetarch {
namespace conn {


// Extends common Connection interface
template<typename AddrT>
class PosixConn : public Connection<AddrT> {
public:
    using Connection<AddrT>::Connection;

    AddrT mmap(AddrT addr, AddrT prot = PROT_READ | PROT_WRITE) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::mmap:" << std::hex << " addr 0x" << addr;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionAddrMmap));
        cmd_mmap_t cmd = { addr, prot };
        detail::vecAppend(vec, cmd);

        this->tr->send(vec);
        auto response = this->tr->recv();
        auto mmapped = detail::vecRead(response);

        if constexpr (utils::is_debug) {
            std::cerr << std::hex << " mmapped to addr 0x" << mmapped;
            if (!mmapped) { std::cerr << " (failed) "; }
            std::cerr << std::dec << std::endl;
        }

        return mmapped;

    }

};


}
}
