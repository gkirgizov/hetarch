#pragma once

#include <cstdint>
#include <asio.hpp>

#include "ConnImplBase.h"
#include "conn_utils.h"
#include "../utils.h"
#include "../addr_typedef.h"


namespace hetarch {
namespace conn {


namespace detail {

std::vector<uint8_t> readBuffer(asio::ip::tcp::socket &socket);
std::size_t readBuffer(asio::ip::tcp::socket &socket, uint8_t* out_buf, std::size_t size);

std::size_t writeBuffer(asio::ip::tcp::socket &socket, const uint8_t* buf, std::size_t size);

}


template<typename AddrT = addr_t>
class TCPConnImpl : public ConnImplBase<AddrT> {
    asio::ip::tcp::socket socket;

    static auto get_socket(const std::string &host, uint16_t port) {
        auto io_service = new asio::io_service(); // socket takes ownership

        asio::ip::tcp::resolver resolver(*io_service);
        asio::ip::tcp::resolver::query query(host, std::to_string(port));
        asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        asio::ip::tcp::socket socket{*io_service};
        asio::connect(socket, endpoint_iterator);
        return std::move(socket);
    }

public:
    TCPConnImpl(const std::string &host, uint16_t port) : socket{get_socket(host, port)} {}
    explicit TCPConnImpl(asio::ip::tcp::socket socket) : socket{std::move(socket)} {}
    ~TCPConnImpl() { close(); }

    void close() {
        socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    }

    std::vector<uint8_t> recv() override {
        return detail::readBuffer(socket);
    }

private:
    AddrT sendImpl(const uint8_t* buf, AddrT size) override {
        return static_cast<AddrT>(detail::writeBuffer(socket, buf, size));
    }

/*    AddrT recvImpl(const uint8_t* buf) {
        uint8_t msg_header_buf[sizeof(msg_header_t)] = { 0 };
        auto header_read = detail::readBuffer(socket, msg_header_buf, sizeof(msg_header_t));
        assert(header_read >= 0, "msg header read failed");
        assert(header_read == sizeof(msg_header_t));
        msg_header_t header = *reinterpret_cast<msg_header_t*>(msg_header_buf);

        auto n_read = readBuffer(socket, buf, header.size);
        assert(n_read > 0, "data read failed");
        assert(n_read == header.size && "read less bytes than expected!");
        return n_read;
    }*/

};


}
}
