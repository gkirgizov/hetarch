#pragma once

//#include <cstdint>

namespace hetarch {
namespace conn {

template<typename AddrT>
class IConnection {
public:
    // todo: what do they return

    virtual AddrT write(AddrT addr, AddrT size, const unsigned char *buf) = 0;

    virtual AddrT read(AddrT addr, AddrT size, unsigned char *buf) = 0;

    virtual bool call(AddrT addr) = 0;

//    virtual ~IConnection() = 0;
};

}
}
