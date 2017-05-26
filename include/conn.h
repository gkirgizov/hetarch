#ifndef HETARCH_CONN_H
#define HETARCH_CONN_H 1

#include <cstdint>

namespace hetarch {
namespace conn {

template<class AddrT>
class IConnection {
public:
    virtual AddrT AddrWrite(AddrT addr, AddrT size, const uint8_t *buf) = 0;
    virtual AddrT AddrRead(AddrT addr, AddrT size, uint8_t *buf) = 0;
    virtual unsigned ScheduleRegular() = 0;
    virtual ~IConnection() {}
};

template<typename T> T vecRead(const std::vector<unsigned char> &vec, int offset) {
    T res = 0;
    for (int i = 0; i < sizeof(T); i++) {
        res |= static_cast<uint64_t>(vec[offset + i]) << (i * 8);
    }
    return res;
}

template<typename T> void vecWrite(std::vector<unsigned char> &vec, int offset, T value) {
    for (int i = 0; i < sizeof(T); i++) {
        vec.insert(vec.begin() + offset + i, static_cast<unsigned char>(value & 0xFF));
        value >>= 8;
    }
}

template<typename T> void vecAppend(std::vector<unsigned char> &vec, T value) {
    for (int i = 0; i < sizeof(T); i++) {
        vec.insert(vec.end(), static_cast<unsigned char>(value & 0xFF));
        value >>= 8;
    }
}

}
}

#endif