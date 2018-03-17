#pragma once

#include <cstdint>
#include <vector>
#include <asio.hpp>

#include "utils.h"
#include "addr_typedef.h"


namespace hetarch {
namespace conn {

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


std::vector<uint8_t> readBuffer(asio::ip::tcp::socket &socket);

std::size_t writeBuffer(asio::ip::tcp::socket &socket, const std::vector<uint8_t> &buffer);

}


}
}
