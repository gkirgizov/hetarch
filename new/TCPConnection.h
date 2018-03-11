#pragma once

#include <sys/mman.h>
#include <cstdint>
#include <asio.hpp>
#include <iostream>

#include "IConnection.h"
#include "conn_utils.h"
#include "utils.h"


namespace hetarch {
namespace conn {


template<typename AddrT>
class TCPConnection : public IConnection<AddrT> {
    std::unique_ptr<asio::ip::tcp::socket> socket;
public:
    TCPConnection(const std::string &host, uint16_t port) {
        asio::io_service io_service;

        asio::ip::tcp::resolver resolver(io_service);
        asio::ip::tcp::resolver::query query(host, std::to_string(port));
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        socket = std::make_unique<asio::ip::tcp::socket>(io_service);
        asio::connect(*socket, endpoint_iterator);
    }

    void close() {
        this->socket->shutdown(asio::ip::tcp::socket::shutdown_both);
    }

    AddrT write(AddrT addr, AddrT size, const unsigned char *buf) override {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::write:"
                      << std::hex << " addr 0x" << addr
                      << std::hex << " size 0x" << size
                      << std::dec << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::AddrWrite));
        detail::vecAppend(vec, addr);
        detail::vecAppend(vec, size);
        vec.insert(vec.end(), buf, buf + size);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        if (response[0]) { throw "PROTOCOL"; }
        return 0;
    }

    AddrT read(AddrT addr, AddrT size, unsigned char *buf) override {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::read:"
                      << std::hex << " addr 0x" << addr
                      << std::hex << " size 0x" << size
                      << std::dec << std::endl;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::AddrRead));
        detail::vecAppend(vec, addr);
        detail::vecAppend(vec, size);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
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
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::Call));
        detail::vecAppend(vec, addr);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        if (auto res = detail::vecRead(response, 0)) {
//            return res;
            return true;
        };
//        return false;
        return true;
    }

    AddrT getBuffer(unsigned idx = 0) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::getBuffer:" << " idx " << idx;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::GetBuffAddr));
        detail::vecAppend(vec, idx);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        auto addr = detail::vecRead(response, 0);

        if constexpr (utils::is_debug) {
            std::cerr << std::hex << " addr 0x" << addr
                      << std::dec << std::endl;
        }

        return addr;
    }

    AddrT mmap(AddrT addr, mmap_rights_t prot = PROT_READ | PROT_WRITE) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::mmap:" << std::hex << " addr 0x" << addr;
        }
        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::AddrMmap));
        detail::vecAppend(vec, addr);
        detail::vecAppend(vec, prot);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
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
