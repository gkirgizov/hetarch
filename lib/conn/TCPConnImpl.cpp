#include "conn/TCPConnImpl.h"

#include <iostream>

#include "conn/ht_proto.h"


namespace hetarch {
namespace conn {


namespace detail {


std::size_t readBuffer(asio::ip::tcp::socket &socket, uint8_t* out_buf, std::size_t size) {
    const std::size_t buf_size = 512;
    size_t collected = 0;
    std::vector<uint8_t> buf(std::min(buf_size, size));
    do {
        if constexpr (utils::is_debug) {
            std::cerr << "conn::detail::readBuffer  : "
                      << "call socket.read_some... ";
        }

        asio::error_code error;
        size_t received = socket.read_some(asio::buffer(buf), error);
        if constexpr (utils::is_debug) {
            std::cerr << " read: " << received << std::endl;
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

        // read data, no more than allowed (size argument)
        auto len = received < size ? received : size;
        std::copy(buf.begin(), buf.begin() + len, out_buf + collected);
        size -= len;
    } while (size > 0);

    if constexpr (utils::is_debug) {
        std::cerr << "conn::detail::readBuffer  : "
                  << "collected " << collected
                  << std::endl;
    }
    return collected;
}


std::vector<uint8_t> readBuffer(asio::ip::tcp::socket &socket) {
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
        size_t received = socket.read_some(asio::buffer(buf), error);
        if constexpr (utils::is_debug) {
            std::cerr << " read: " << received
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
        auto len = received;
        if (expected == 0) { // read command header
            if constexpr (utils::is_debug) {
                std::cerr << "conn::detail::readBuffer  : "
                          << "reading msg_header...";
            }
            // tmp
            assert(len >= sizeof(msg_header_t));

            auto cmd_header = vecRead<msg_header_t>(buf);
            if (!check_magic(&cmd_header)) break; // got some noise..

            beg += sizeof(cmd_header);
            len -= sizeof(cmd_header);
            expected = cmd_header.size;

            if constexpr (utils::is_debug) {
                std::cerr << " expecting " << expected << " bytes" << std::endl;
            }
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

std::size_t writeBuffer(asio::ip::tcp::socket &socket, const uint8_t* buf, std::size_t size) {
    std::vector<uint8_t> dataToSend;

    msg_header_t cmd_header = get_msg_header(static_cast<addr_t>(size));
    detail::vecAppend(dataToSend, cmd_header);
    dataToSend.insert(dataToSend.end(), buf, buf + size);

    if constexpr (utils::is_debug) {
        std::cerr << "conn::detail::writeBuffer :"
                  << " payload size " << cmd_header.size
                  << " total size " << dataToSend.size()
                  << " call asio::write... ";
    }
    asio::error_code ignored_error;
    std::size_t written = asio::write(socket, asio::buffer(dataToSend), ignored_error);
    if constexpr (utils::is_debug) {
        std::cerr << " written " << written << std::endl;
    }

    if (written != dataToSend.size()) {
        std::cerr << "Warning: conn::detail::writeBuffer :"
                  << " written != dataToSend.size()"
                  << " (" << written << " != " << dataToSend.size() << ")"
                  << std::endl;
    }

    return written;
}

}


}
}
