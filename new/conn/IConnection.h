#pragma once

#include <cstdint>
#include <string.h>
#include <sys/mman.h>

#include "Transmission.h"
#include "ht_proto.h"
#include "../utils.h"


namespace hetarch {
namespace conn {


template<typename AddrT>
class IConnection {
public:
    virtual AddrT write(AddrT addr, AddrT size, const uint8_t* buf) = 0;

    virtual AddrT read(AddrT addr, AddrT size, uint8_t* buf) = 0;

    virtual bool call(AddrT addr) = 0;

    virtual bool schedule(AddrT addr, AddrT ms_delay) = 0;

    virtual ~IConnection() = default;
};


template<typename AddrT>
class CmdProtocol : public IConnection<AddrT> {
    ITransmission<AddrT>& tr;
public:
    explicit CmdProtocol(ITransmission<AddrT>& tr) : tr{tr} {}

    bool echo(AddrT size, const uint8_t* in_buf, uint8_t* out_buf) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::write:"
                      << " data size " << size
                      << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionEcho));
        vec.insert(vec.end(), in_buf, in_buf + size);

        tr.send(vec);
        auto response = tr.recv();
        assert(response.size() == size);
        memcpy(out_buf, response.data(), size);

//        tr.recv(size, out_buf);
        return strncmp((const char*)in_buf, (const char*)out_buf, size) == 0;
    }
    bool echo(const char* msg) {
        const std::size_t msg_size = strlen(msg);
        const std::size_t buf_size = 512;
        assert(msg_size <= buf_size);

        uint8_t out_buf[buf_size];
        return echo(msg_size, reinterpret_cast<const uint8_t*>(msg), out_buf);
    }

    AddrT write(AddrT addr, AddrT size, const uint8_t* buf) override {
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
        auto written = detail::vecRead<AddrT>(response);
        if constexpr (utils::is_debug) {
            std::cerr << "conn::write:" << " returned: written " << written << std::endl;
        }
        return written;
    }

    AddrT read(AddrT addr, AddrT size, uint8_t* buf) override {
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
        if constexpr (utils::is_debug) {
            std::cerr << "conn::read:"
                      << " got response: size " << response.size()
                      << ": " << (response.size() ? "ok." : "error!")
                      << std::endl;
        }
        std::copy(response.begin(), response.end(), buf);
        return response.size();
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
        auto res = detail::vecRead<cmd_ret_code_t>(vec);
        if constexpr (utils::is_debug) {
            std::cerr << "conn::call: "
                      << (res == ErrCall ? "error!" : "ok.")
                      << std::endl;
        }
        return res != ErrCall;
    }

    bool schedule(AddrT addr, AddrT ms_delay) override {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::schedule:"
                      << std::hex << " addr 0x" << addr
                      << std::dec << " ms " << ms_delay
                      << std::dec << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionSchedule));
        cmd_schedule_t cmd = { addr, ms_delay };
        detail::vecAppend(vec, cmd);

        tr.send(vec);
        auto response = tr.recv();
        auto res = detail::vecRead<cmd_ret_code_t>(vec);
        if constexpr (utils::is_debug) {
            std::cerr << "conn::schedule: "
                      << (res ? "ok." : "error!")
                      << std::endl;
        }
        return res;
    }

    AddrT getBuffer(uint8_t idx = 0, AddrT size = 1024) {
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
        auto addr = detail::vecRead<AddrT>(response);
        if constexpr (utils::is_debug) {
            std::cerr << std::hex << " got addr 0x" << addr
                      << ": " << (addr ? "ok." : "error!")
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

    AddrT call2(AddrT addr, AddrT x1, AddrT x2) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::call2:"
                      << std::hex << " addr 0x" << addr
                      << std::dec << "; x1=" << x1 << "; x2=" << x2
                      << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(ActionCall2));
        cmd_call2_t cmd = { addr, x1, x2 };
        detail::vecAppend(vec, cmd);

        tr.send(vec);
        auto response = tr.recv();
        auto res = detail::vecRead<AddrT>(response);
        if constexpr (utils::is_debug) {
            std::cerr << "conn::call2: returned: " << std::dec << res << std::endl;
        }
        return res;

    }
};

}
}
