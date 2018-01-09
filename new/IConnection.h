#pragma once

//#include <cstdint>

namespace hetarch {
namespace conn {

template<typename AddrT>
class IConnection {
public:
    virtual AddrT write(AddrT addr, AddrT size, const char *buf) = 0;

    virtual AddrT read(AddrT addr, AddrT size, char *buf) = 0;

//    virtual ~IConnection() = 0;
};

}
}
