#pragma once

#include <cstdint>

namespace hetarch {
namespace conn {

template<typename AddrT>
class IConnection {
public:
    virtual AddrT write(AddrT addr, AddrT size, const uint8_t *buf) = 0;

    virtual AddrT read(AddrT addr, AddrT size, uint8_t *buf) = 0;

    virtual ~IConnection() = 0;
};

}
}
