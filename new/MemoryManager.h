#pragma once


#include <vector>
#include <memory>

#include "boost/interprocess/mem_algo/simple_seq_fit.hpp"


namespace hetarch {
namespace mem {

// maybe do something like bitmask?
enum class MemType {
    ReadOnly,
    ReadPreferred,
    ReadWrite,
};


template<typename AddrT>
struct MemRegion {
    MemRegion(const AddrT start, const AddrT size, const MemType memType)
    : size(size), start(start), end(start+size), memType(memType)
    {}

    const AddrT size;
    const AddrT start;
    const AddrT end;
    const MemType memType;
};



template<typename AddrT>
class MemManager {

    // for Boost.Interprocess
    struct mutex_family {
        typedef boost::interprocess::interprocess_mutex mutex_type;
        typedef boost::interprocess::interprocess_recursive_mutex recursive_mutex_type;
    };

    using void_ptr = void *;
    // As we allocate memory on the different machine,
    // ptr type on 'master' machine (void_ptr) must allow
    // to address its (different machine's) memory.
    // So, this assert states exactly this.
    static_assert(sizeof(void_ptr) >= sizeof(AddrT));
    using m_impl_t = boost::interprocess::simple_seq_fit<mutex_family, void_ptr>;
    using size_type = typename m_impl_t::size_type;

//    constexpr auto m_mcoef = sizeof(void_ptr) / sizeof(AddrT);

    m_impl_t m_impl;

public:
    // MemManager will allocate memory from provided regions
    explicit MemManager(const MemRegion<AddrT> &memAvailable)
    : m_impl(static_cast<size_type>(memAvailable.size), static_cast<size_type>(AddrT()))
    {}

//    MemManager(const std::vector<MemRegion<AddrT>> &memAvailable);

    MemRegion<AddrT> alloc(AddrT memSize, MemType memType);

    // to control memory layout manually -- 'register' memory usage
    // todo: check if it is okey for interface, encapsulation etc.
    bool tryAlloc(const MemRegion<AddrT> &memRegion);

//     todo: should we be quite strict regarding invalid input? are only allocated reg-s allowed?
    void free(const MemRegion<AddrT> &memRegion);

};



template<typename AddrT>
MemRegion<AddrT> MemManager<AddrT>::alloc(AddrT memSize, MemType memType) {
    // won't compile with -fpermissive when sizeof(AddrT) < sizeof(void_ptr) because of loss of precision
    if (auto offset = reinterpret_cast<AddrT>(m_impl.allocate(memSize))) {

        AddrT segmentStart = 0;

        AddrT start = offset + segmentStart;
        return MemRegion<AddrT>(start, memSize, memType);
    } else {
        // todo: handle not enough memory (when offset == 0)
    }
}

}
}

