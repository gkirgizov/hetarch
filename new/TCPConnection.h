#pragma once

#include <cstdint>
#include <limits>
#include <asio.hpp>
#include <iostream>

#include "IConnection.h"
#include "utils.h"


namespace hetarch {
namespace conn {


using action_int_t = uint16_t;
enum class Actions : action_int_t {
    AddrRead = 1,
    AddrWrite = 2,
    GetBuffAddr = 3,
    Call = 4,
};

using addr_t = HETARCH_TARGET_ADDRT;

namespace detail {

// Return ref to data owned by vec
template<typename T = addr_t>
inline const T& vecRead(const std::vector<unsigned char> &vec, int offset = 0) {
    return utils::fromBytes<T>(vec.data() + offset);
}

template<typename T>
inline void vecWrite(std::vector<unsigned char> &vec, int offset, T value) {
    const unsigned char* bytes = utils::toBytes(value);
    vec.insert(vec.begin() + offset, bytes, bytes + sizeof(T));
}

template<typename T>
inline void vecAppend(std::vector<unsigned char> &vec, T value) {
    const unsigned char* bytes = utils::toBytes(value);
    vec.insert(vec.end(), bytes, bytes + sizeof(T));
}

}

namespace detail {

std::vector<uint8_t> readBuffer(asio::ip::tcp::socket &socket);

void writeBuffer(asio::ip::tcp::socket &socket, const std::vector<uint8_t> &buffer);

}


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

    AddrT getBuffer(unsigned idx) {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::getBuffer:" << " idx " << idx;
        }

        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::GetBuffAddr));
        detail::vecAppend(vec, 0);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        auto addr = detail::vecRead(response, 0);

        if constexpr (utils::is_debug) {
            std::cerr << std::hex << " addr 0x" << addr
                      << std::dec << std::endl;
        }

        return addr;
    }

};


}
}
