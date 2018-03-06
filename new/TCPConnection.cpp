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
        asio::error_code error;
        buf = std::vector<uint8_t>(512);

        size_t len = socket.read_some(asio::buffer(buf), error);

        if (error == asio::error::eof) {
            break; // Connection closed cleanly by peer.
        } else if (error) {
            if constexpr (utils::is_debug) {
                std::cerr << "readBuffer: error: " << error << std::endl;
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
    return collected;
}

void writeBuffer(tcp::socket &socket, const std::vector<uint8_t> &buffer) {
    asio::error_code ignored_error;
    std::vector<unsigned char> dataToSend;
    vecWrite(dataToSend, 0, static_cast<addr_t>(buffer.size()));
    dataToSend.insert(dataToSend.end(), buffer.begin(), buffer.end());
    asio::write(socket, asio::buffer(dataToSend), ignored_error);
}

}


}
}
