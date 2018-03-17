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
    std::unique_ptr<asio::ip::tcp::socket> socket;
public:
    TCPTrans(const std::string &host, uint16_t port) {
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

    AddrT send(const std::vector<uint8_t>& buf) override {
        return static_cast<AddrT>(detail::writeBuffer(*socket, buf));
    }

    std::vector<uint8_t> recv() override {
        return detail::readBuffer(*socket);
    }
};


}
}
