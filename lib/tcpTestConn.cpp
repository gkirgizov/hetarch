#include <cstdint>
#include <asio.hpp>
#include <algorithm>

#include "../include/TCPTestConn.h"

using asio::ip::tcp;

namespace hetarch {
namespace conn {

TCPTestConnection::TCPTestConnection(const std::string &host, uint16_t port) {
    asio::io_service io_service;

    tcp::resolver resolver(io_service);
    tcp::resolver::query query(host, std::to_string(port));
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    socket = std::unique_ptr<tcp::socket>(new tcp::socket(io_service));
    asio::connect(*socket.get(), endpoint_iterator);
}

uint64_t TCPTestConnection::AddrWrite(uint64_t addr, uint64_t size, const unsigned char *buf) {
    std::vector<uint8_t> vec;
    vecAppend<uint32_t>(vec, static_cast<uint32_t>(hetarch::conn::Actions::AddrWrite));
    vecAppend<uint64_t>(vec, addr);
    vecAppend<uint64_t>(vec, size);
    vec.insert(vec.end(), buf, buf + size);
    writeBuffer(*socket.get(), vec);
    auto response = readBuffer(*socket.get());
    if (response[0]) { throw "PROTOCOL"; }
    return 0;
}

uint64_t TCPTestConnection::AddrRead(uint64_t addr, uint64_t size, unsigned char *buf) {
    std::vector<uint8_t> vec;
    vecAppend<uint32_t>(vec, static_cast<uint32_t>(hetarch::conn::Actions::AddrRead));
    vecAppend<uint64_t>(vec, addr);
    vecAppend<uint64_t>(vec, size);
    writeBuffer(*socket.get(), vec);
    auto response = readBuffer(*socket.get());
    std::copy(response.begin(), response.end(), buf);
}

uint64_t TCPTestConnection::GetBuff(uint32_t idx) {
    std::vector<uint8_t> vec;
    vecWrite<uint32_t>(vec, 0, static_cast<uint32_t>(hetarch::conn::Actions::GetBuffAddr));
    vecWrite<uint32_t>(vec, 4, 0);
    writeBuffer(*socket.get(), vec);
    auto response = readBuffer(*socket.get());
    uint64_t addr = vecRead<uint64_t>(response, 0);
    return addr;
}

uint32_t TCPTestConnection::Call(uint64_t addr) {
    std::vector<uint8_t> vec;
    vecAppend<uint32_t>(vec, static_cast<uint32_t>(hetarch::conn::Actions::Call));
    vecAppend<uint64_t>(vec, addr);
    writeBuffer(*socket.get(), vec);
    auto response = readBuffer(*socket.get());
    uint32_t res = vecRead<uint32_t>(response, 0);
    return res;
}

unsigned TCPTestConnection::ScheduleRegular() {
    throw "unimplemented";
}

std::vector<uint8_t> readBuffer(tcp::socket &socket) {
    size_t expected = 0;
    std::vector<uint8_t> collected;
    std::vector<uint8_t> buf;
    for (;;) {
        asio::error_code error;
        buf = std::vector<uint8_t>(512);

        size_t len = socket.read_some(asio::buffer(buf), error);

        if (error == asio::error::eof) {
            break; // Connection closed cleanly by peer.
        } else if (error) {
            throw asio::system_error(error); // Some other error.
        }

        if (expected == 0) {
            expected = vecRead<uint64_t>(buf, 0);
            buf = std::vector<uint8_t>(buf.begin() + 8, buf.begin() + len);
            len -= 8;
        }
        collected.insert(collected.end(), buf.begin(), buf.begin() + len);

        expected -= len;
        if (!expected) { break; }
    }
    return collected;
}

void writeBuffer(tcp::socket &socket, const std::vector<uint8_t> &buffer) {
    asio::error_code ignored_error;
    std::vector<unsigned char> dataToSend;
    vecWrite<uint64_t>(dataToSend, 0, (uint64_t)buffer.size());
    dataToSend.insert(dataToSend.end(), buffer.begin(), buffer.end());
    asio::write(socket, asio::buffer(dataToSend), ignored_error);
}

void TCPTestConnection::Close() {
    this->socket->shutdown(asio::ip::tcp::socket::shutdown_both);
}

}
}