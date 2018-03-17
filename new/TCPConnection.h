#pragma once

#include <cstdint>
#include <asio.hpp>

#include "Transmission.h"
#include "conn_utils.h"
#include "utils.h"


namespace hetarch {
namespace conn {


namespace detail {

std::vector<uint8_t> readBuffer(asio::ip::tcp::socket &socket);

std::size_t writeBuffer(asio::ip::tcp::socket &socket, const std::vector<uint8_t> &buffer);

}


template<typename AddrT>
class TCPTrans: public ITransmission<AddrT> {
    asio::ip::tcp::socket socket;

    static auto get_socket(const std::string &host, uint16_t port) {
        asio::io_service io_service;

        asio::ip::tcp::resolver resolver(io_service);
        asio::ip::tcp::resolver::query query(host, std::to_string(port));
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        asio::ip::tcp::socket socket{io_service};
        asio::connect(socket, endpoint_iterator);
        return socket;
    }

public:
    TCPTrans(const std::string &host, uint16_t port) : socket{get_socket(host, port)} {}
    explicit TCPTrans(asio::ip::tcp::socket socket) : socket{std::move(socket)} {}

    void close() {
        socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    }

    AddrT send(const std::vector<uint8_t>& buf) override {
        return static_cast<AddrT>(detail::writeBuffer(socket, buf));
    }

    std::vector<uint8_t> recv() override {
        return detail::readBuffer(socket);
    }
};


}
}
