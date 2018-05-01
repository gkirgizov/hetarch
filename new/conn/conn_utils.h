#pragma once

#include <cstdint>
#include <vector>

#include "../utils.h"
#include "../addr_typedef.h"


namespace hetarch {
namespace conn {

namespace detail {

template<typename T = addr_t>
inline const T vecRead(const std::vector<unsigned char> &vec, int offset = 0) {
    return utils::fromBytes<T>(vec.data() + offset);
}

template<typename T>
inline void vecWrite(std::vector<unsigned char> &vec, int offset, T value) {
    unsigned char bytes[sizeof(T)];
    utils::toBytes(value, bytes);
    vec.insert(vec.begin() + offset, bytes, bytes + sizeof(T));
}

template<typename T>
inline void vecAppend(std::vector<unsigned char> &vec, T value) {
    unsigned char bytes[sizeof(T)];
    utils::toBytes(value, bytes);
    vec.insert(vec.end(), bytes, bytes + sizeof(T));
}

}


}
}
