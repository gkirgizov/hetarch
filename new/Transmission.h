#pragma once

#include "conn_utils.h"


namespace hetarch {
namespace conn {


template<typename AddrT>
class ITransmission {
public:
    virtual std::vector<uint8_t> recv() = 0;
    virtual AddrT send(const std::vector<uint8_t>& buf) = 0;

/*    virtual AddrT send(const uint8_t* buf, AddrT size) = 0;

    inline AddrT send(const std::vector<uint8_t>& buf) {
        send(buf.data(), static_cast<AddrT>(buf.size()));
    };

    template<std::size_t N>
    inline AddrT send(const std::array<uint8_t, N>& buf) {
        send(buf.data(), static_cast<AddrT>(buf.size()));
    };*/
};


}
}
