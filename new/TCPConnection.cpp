#include "TCPConnection.h"

#include <cstdint>


using asio::ip::tcp;

namespace hetarch {
namespace conn {


namespace detail {

std::vector<uint8_t> readBuffer(tcp::socket &socket) {
    size_t expected = 0;
    std::vector<uint8_t> collected;
    std::vector<uint8_t> buf;
    do {
        buf = std::vector<uint8_t>(512);

        if constexpr (utils::is_debug) {
            std::cerr << "conn::detail::readBuffer  : "
                      << "call socket.read_some... ";
        }
        asio::error_code error;
        size_t len = socket.read_some(asio::buffer(buf), error);
        if constexpr (utils::is_debug) {
            std::cerr << " called, read: " << len
                      << std::endl;
        }

        if (error == asio::error::eof) {
            break; // Connection closed cleanly by peer.
        } else if (error) {
            if constexpr (utils::is_debug) {
                std::cerr << "conn::detail::readBuffer  : "
                          << "asio::error: " << error << std::endl;
            }
            throw asio::system_error(error); // Some other error.
        }

        auto beg = buf.begin();
        if (expected == 0) { // read total size of expected data
            expected = vecRead(buf, 0);
            beg += sizeof(addr_t);
            len -= sizeof(addr_t);
        }
        // read data
        collected.insert(collected.end(), beg, beg + len);
        expected -= len;
    } while (expected > 0);
    if constexpr (utils::is_debug) {
        std::cerr << "conn::detail::readBuffer  : "
                  << "return collected of size " << collected.size()
                  << std::endl;
    }
    return collected;
}

void writeBuffer(tcp::socket &socket, const std::vector<uint8_t> &buffer) {
    std::vector<unsigned char> dataToSend;
    vecWrite(dataToSend, 0, static_cast<addr_t>(buffer.size()));
    dataToSend.insert(dataToSend.end(), buffer.begin(), buffer.end());

    if constexpr (utils::is_debug) {
        std::cerr << "conn::detail::writeBuffer : "
                  << "call asio::write... ";
    }
    asio::error_code ignored_error;
    std::size_t written = asio::write(socket, asio::buffer(dataToSend), ignored_error);
    if constexpr (utils::is_debug) {
        std::cerr << " called, written: " << written
                  << std::endl;
    }

    if (written != dataToSend.size()) {
        std::cerr << "Warning: conn::detail::writeBuffer : "
                  << "written != dataToSend.size()"
                  << " (" << written << " != " << dataToSend.size() << ")"
                  << std::endl;
    }
}

}


}
}
