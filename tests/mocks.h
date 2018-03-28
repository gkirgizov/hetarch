#pragma once

#include <iostream>

#include <boost/format.hpp>

#include "../new/conn/IConnection.h"
#include "../new/MemoryManager.h"


namespace hetarch {
namespace mock {

template<typename AddrT>
class MockConnection : public conn::IConnection<AddrT> {
public:
    AddrT write(AddrT addr, AddrT size, const char *buf) override {
        std::cerr << boost::format("IConnection::write(%x, %d, %x): ") % addr % size % buf;
        std::cerr.write(buf, size);
        std::cerr << std::endl;
        return size;
    }

    AddrT read(AddrT addr, AddrT size, char *buf) override {
        std::cerr << boost::format("IConnection::read(%x, %d, %x)") % addr % size % buf;
        std::cerr << std::endl;
        return size;
    }

};


template<typename AddrT>
class MockMemManager : public mem::MemManager<AddrT> {
public:
    explicit MockMemManager(const AddrT defaultAlignment = sizeof(AddrT)) : mem::MemManager<AddrT>(defaultAlignment) {}

private:
    mem::MemRegion<AddrT> allocImpl(AddrT memSize, mem::MemType memType, AddrT alignment) override {
        return mem::MemRegion<AddrT>(0, memSize, memType);
    }

    mem::MemRegion<AddrT> tryAllocImpl(const mem::MemRegion<AddrT> &memRegion, AddrT alignment) override {
        return memRegion;
    }

    void freeImpl(const mem::MemRegion<AddrT> &memRegion) override {
    }


    bool unsafeFreeImpl(const mem::MemRegion<AddrT> &memRegion) override {
        return true;
    }

};

}
}
