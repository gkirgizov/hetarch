#pragma once

#include <cstdint>
#include <sys/mman.h>

#include "Transmission.h"
#include "ht_proto.h"
#include "utils.h"


namespace hetarch {
namespace conn {


template<typename AddrT>
class IConnection {
public:
    virtual AddrT write(AddrT addr, AddrT size, const unsigned char *buf) = 0;

    virtual AddrT read(AddrT addr, AddrT size, unsigned char *buf) = 0;

    virtual bool call(AddrT addr) = 0;
};


template<typename AddrT>
class CmdProtocol : public IConnection<AddrT> {
    ITransmission<AddrT>& tr;
public:
    explicit CmdProtocol(ITransmission<AddrT>& tr) : tr{tr} {}

    AddrT write(AddrT addr, AddrT size, const unsigned char *buf) override {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::write:"
                      << std::hex << " addr 0x" << addr
                      << std::dec << " size " << size
                      << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionWrite));
        cmd_write_t cmd = { addr, size };
        detail::vecAppend(vec, cmd);
        vec.insert(vec.end(), buf, buf + size);

        tr.send(vec);
        auto response = tr.recv();
        if (response[0]) { throw "PROTOCOL"; }
        return 0;
    }

    AddrT read(AddrT addr, AddrT size, unsigned char *buf) override {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::read:"
                      << std::hex << " addr 0x" << addr
                      << std::dec << " size " << size
                      << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionRead));
        cmd_read_t cmd = { addr, size };
        detail::vecAppend(vec, cmd);

        tr.send(vec);
        auto response = tr.recv();
        std::copy(response.begin(), response.end(), buf);
        return 0;
    }

    bool call(AddrT addr) override {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::call:"
                      << std::hex << " addr 0x" << addr
                      << std::dec << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionCall));
        cmd_call_t cmd = { addr };
        detail::vecAppend(vec, cmd);

        tr.send(vec);
        auto response = tr.recv();
        if (auto res = detail::vecRead(response, 0)) {
//            return res;
            return true;
        };
//        return false;
        return true;
    }

    AddrT getBuffer(uint8_t idx = 0, AddrT size = 1024 * 1024) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::getBuffer:"
                      << " idx " << static_cast<unsigned>(idx)
                      << " size " << size
                      << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionGetBuffer));
        cmd_get_buffer_t cmd = { idx, size };
        detail::vecAppend(vec, cmd);

        tr.send(vec);
        auto response = tr.recv();
        auto addr = detail::vecRead(response, 0);

        if constexpr (utils::is_debug) {
            std::cerr << std::hex << " got addr 0x" << addr
                      << std::dec << std::endl;
        }

        return addr;
    }

    AddrT mmap(AddrT addr, AddrT prot = PROT_READ | PROT_WRITE) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::mmap:" << std::hex << " addr 0x" << addr;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionAddrMmap));
        cmd_mmap_t cmd = { addr, prot };
        detail::vecAppend(vec, cmd);

        tr.send(vec);
        auto response = tr.recv();
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
