#pragma once

#include <cstdint>
#include <asio.hpp>

#include "IConnection.h"


namespace hetarch {
namespace conn {


using action_int_t = uint16_t;
enum class Actions : action_int_t {
//enum class Actions {
    AddrRead = 1,
    AddrWrite = 2,
    GetBuffAddr = 3,
    Call = 4,
};

//using addr_t = uint64_t;
using addr_t = HETARCH_TARGET_ADDRT;

namespace detail {

template<typename T> T vecRead(const std::vector<unsigned char> &vec, int offset) {
    T res = 0;
    for (int i = 0; i < sizeof(T); i++) {
        res |= static_cast<addr_t>(vec[offset + i]) << (i * sizeof(addr_t));
    }
    return res;
}

template<typename T> void vecWrite(std::vector<unsigned char> &vec, int offset, T value) {
    for (int i = 0; i < sizeof(T); i++) {
        vec.insert(vec.begin() + offset + i, static_cast<unsigned char>(value & 0xFF));
        value >>= sizeof(addr_t);
    }
}

template<typename T> void vecAppend(std::vector<unsigned char> &vec, T value) {
    for (int i = 0; i < sizeof(T); i++) {
        vec.insert(vec.end(), static_cast<unsigned char>(value & 0xFF));
        value >>= sizeof(addr_t);
    }
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
    ~TCPConnection() { close(); }

    void close() {
        this->socket->shutdown(asio::ip::tcp::socket::shutdown_both);
    }

    AddrT write(AddrT addr, AddrT size, const char *buf) override {
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

    AddrT read(AddrT addr, AddrT size, char *buf) override {
        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::AddrRead));
        detail::vecAppend(vec, addr);
        detail::vecAppend(vec, size);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        std::copy(response.begin(), response.end(), buf);
    }

    bool call(AddrT addr) override {
        std::vector<uint8_t> vec;
        detail::vecAppend(vec, static_cast<action_int_t>(hetarch::conn::Actions::Call));
        detail::vecAppend(vec, addr);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        if (auto res = detail::vecRead<uint32_t>(response, 0)) {
//            return res;
            return true;
        };
//        return false;
        return true;
    }

    AddrT getBuffer(unsigned idx) {
        std::vector<uint8_t> vec;
        detail::vecWrite(vec, 0, static_cast<action_int_t>(hetarch::conn::Actions::GetBuffAddr));
        detail::vecWrite(vec, sizeof(action_int_t), 0);
        detail::writeBuffer(*socket, vec);
        auto response = detail::readBuffer(*socket);
        auto addr = detail::vecRead<addr_t>(response, 0);
        return addr;
    }

};


}
}
