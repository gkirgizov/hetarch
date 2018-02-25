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

}


}
}
