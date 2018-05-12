#pragma once

#include "conn_utils.h"


namespace hetarch {
namespace conn {


template<typename AddrT>
class ConnImplBase {
//    virtual AddrT recvImpl(uint8_t* buf) = 0;
    virtual AddrT sendImpl(const uint8_t* buf, AddrT size) = 0;

public:
//    virtual AddrT recv(uint8_t* buf) = 0;
    virtual std::vector<uint8_t> recv() = 0;

    inline AddrT send(const uint8_t* buf, AddrT size) {
        return sendImpl(buf, size);
    };
    inline AddrT send(const std::vector<uint8_t>& buf) {
        return sendImpl(buf.data(), static_cast<AddrT>(buf.size()));
    };
    template<std::size_t N>
    inline AddrT send(const std::array<uint8_t, N>& buf) {
        return sendImpl(buf.data(), static_cast<AddrT>(buf.size()));
    };

    virtual ~ConnImplBase() = default;
};


}
}
