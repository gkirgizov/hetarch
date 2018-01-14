#pragma once


#include <vector>
#include <memory>

#include <unordered_map>
#include <set>


namespace hetarch {
namespace mem {

// maybe do something like bitmask?
enum class MemType {
    Undefined,
    ReadOnly,
    ReadPreferred,
    ReadWrite,
};


template<typename AddrT>
struct MemRegion {
    explicit MemRegion() : size{0}, start{0}, end{0}, memType{MemType::Undefined} {};  // for constructing false-y value

    MemRegion(const AddrT start, const AddrT size, const MemType memType)
    : size{size}, start{start}, end{start+size}, memType{memType}
    {}

    const AddrT size;
    const AddrT start;
    const AddrT end;
    const MemType memType;

    explicit operator bool() const { return size != 0; }
};


template<typename AddrT>
class MemManager {

    using void_ptr = void *;
    // As we allocate memory on the different machine,
    // ptr type on 'master' machine (void_ptr) must allow
    // to address its (different machine's) memory.
    // So, this assert states exactly this.
    static_assert(sizeof(void_ptr) >= sizeof(AddrT));
public:
    const AddrT defaultAlignment;

    static inline bool aligned(AddrT x, AddrT alignment) { return x % alignment == 0; }
    static inline AddrT alignDown(AddrT x, AddrT alignment) { return x - x % alignment; }
    static inline AddrT alignUp(AddrT x, AddrT alignment) {
        auto r = x % alignment;
        return r != 0 ? x - r + alignment : x;
    }

public:
    explicit MemManager(const AddrT defaultAlignment) : defaultAlignment{defaultAlignment} {
        assert(defaultAlignment % 2 == 0 || defaultAlignment == 1);
    }

    MemRegion<AddrT> alloc(AddrT memSize, MemType memType, AddrT alignment) {
        return allocImpl(memSize, memType, alignment);
    };
    MemRegion<AddrT> alloc(AddrT memSize, MemType memType) {
        return allocImpl(memSize, memType, defaultAlignment);
    }

    /// Allows to control memory layout manually, i.e. 'register' memory usage
    /// \param memRegion
    /// \param alignment
    /// \return actually allocated region (takes @param alignment into consideration)
    MemRegion<AddrT> tryAlloc(const MemRegion<AddrT> &memRegion, AddrT alignment) {
        return tryAllocImpl(memRegion, alignment);
    }
    MemRegion<AddrT> tryAlloc(const MemRegion<AddrT> &memRegion) {
        return tryAllocImpl(memRegion, defaultAlignment);
    }

    // todo: what to do with alignment here?
    void free(const MemRegion<AddrT> &memRegion) {
        freeImpl(memRegion);
    };

    /// Free without any checks. Can be easily misused. Only for the most confident callers who know definetly what they've allocated.
    /// \param memRegion previously allocated region
    /// \return true if memRegion has consistent MemType
    bool unsafeFree(const MemRegion<AddrT> &memRegion) {
        return unsafeFreeImpl(memRegion);
    }

    virtual ~MemManager() = default;

private:
    virtual MemRegion<AddrT> allocImpl(AddrT memSize, MemType memType, AddrT alignment) = 0;
    virtual MemRegion<AddrT> tryAllocImpl(const MemRegion<AddrT> &memRegion, AddrT alignment) = 0;
    virtual void freeImpl(const MemRegion<AddrT> &memRegion) = 0;
    virtual bool unsafeFreeImpl(const MemRegion<AddrT> &memRegion) = 0;

};


namespace mem_mgr_impl {

struct cmp_size_less {
    template<typename AddrT>
    bool operator()(const MemRegion<AddrT> &a, const MemRegion<AddrT> &b) {
        // small to big: best fit
        return a.size < b.size;
    }
};

struct cmp_size_more {
    template<typename AddrT>
    bool operator()(const MemRegion<AddrT> &a, const MemRegion<AddrT> &b) {
        // big to small: worst fit
        return a.size > b.size;
    }
};

struct cmp_start_less {
    template<typename AddrT>
    bool operator()(const MemRegion<AddrT> &a, const MemRegion<AddrT> &b) {
        // natural ordering
        return a.start < b.start;
    }
};

using best_fit = cmp_size_less;
//using worst_fit = cmp_size_more;
using seq_fit = cmp_start_less;

}


template<typename AddrT>
class MemManagerBestFit: public MemManager<AddrT> {

    using impl_cont_t = std::multiset<MemRegion<AddrT>, mem_mgr_impl::best_fit>;
    std::unordered_map<MemType, impl_cont_t> freeRegions{};

    // todo: coalesce blocks when natural (by-address) ordering
    void insertRegion(MemRegion<AddrT> newRegion, impl_cont_t &regions) {
        regions.insert(newRegion);
    }

public:
    // move-only type
    MemManagerBestFit(const MemManagerBestFit &) = delete;
    MemManagerBestFit &operator=(MemManagerBestFit &) = delete;

    explicit MemManagerBestFit(MemRegion<AddrT> availableMem, AddrT defaultAlignment = sizeof(AddrT)) : MemManager<AddrT>{defaultAlignment} {
        freeRegions[availableMem.memType].insert(std::move(availableMem));
    }

    explicit MemManagerBestFit(const std::vector<MemRegion<AddrT>> &availableMem, AddrT defaultAlignment = sizeof(AddrT)) : MemManager<AddrT>{defaultAlignment} {
        for (MemRegion<AddrT> memRegion : availableMem) {
            freeRegions[memRegion.memType].insert(std::move(memRegion));
        }
    }

    void printMemRegions(std::ostream &stream) const {
        for (auto kv : freeRegions) {
            auto memType = kv.first;
            int i = 0;
            for (auto mr : kv.second) {
                stream << "n:" << i++
                       << " memType:" << static_cast<int>(memType)
                       << " st:" << mr.start
                       << " end:" << mr.end
                       << " sz:" << mr.size
                       << std::endl;
            }
        }
    }

private:
    MemRegion<AddrT> allocImpl(AddrT memSize, MemType memType, AddrT alignment) override {
        // todo: if there's no memory of such type, then fallback to less-restrictive memType (don't forget to rewrite memType)
        auto& regions = freeRegions[memType];

        memSize = this->alignUp(memSize, alignment);

        MemRegion<AddrT> tempRegion(0, memSize, memType);
        auto it = regions.lower_bound(tempRegion);
        if (it != std::end(regions)) {
            MemRegion<AddrT> splittedRegion(it->start + memSize, it->size - memSize, memType);
            MemRegion<AddrT> foundRegion(it->start, memSize, memType);
            regions.erase(it);
            regions.insert(splittedRegion);
//            insertRegion(splittedRegion, regions);
            return foundRegion;
        }

        return MemRegion<AddrT>();
    }

    MemRegion<AddrT> tryAllocImpl(const MemRegion<AddrT> &memRegion, AddrT alignment) override {
        // todo: BIG TODO: there're already several types of error. (OutOfMem; non-aligned memRegion). exceptions can be bad. what about error codes? is it bad in any sense?
        if (!this->aligned(memRegion.start, alignment)) {
            return MemRegion<AddrT>{};
        }

        auto memType = memRegion.memType;
        auto& regions = freeRegions[memType];

        MemRegion<AddrT> actualMemRegion{memRegion.start, this->alignUp(memRegion.size, alignment), memType};

        auto ms = actualMemRegion.start, me = actualMemRegion.end;
        for (auto it = std::begin(regions); it != std::end(regions); ++it) {
            auto is = it->start, ie = it->end;

            // if there is any intersection then it's impossible to find fitting block, hence additional checks
            if (ms >= is) {
                if (me <= ie) {
                    regions.erase(it);
                    if (ms > is) {
                        regions.insert(MemRegion<AddrT>(is, ms - is, memType));
                    }
                    if (me < ie) {
                        regions.insert(MemRegion<AddrT>(me, ie - me, memType));
                    }
                    return actualMemRegion;
                }
                return MemRegion<AddrT>{};
            } else if (me > is) {
                return MemRegion<AddrT>{};
            }
        }
        return MemRegion<AddrT>{};
    }

    // todo: maybe somehow disallow freeing arbitrary memRegions? only the ones which were allocated.
    void freeImpl(const MemRegion<AddrT> &memRegion) override {
        // find ALL overlapped blocks
        // among them find 2 'border' blocks
        // find overlaps with them
        // split them correspondingly
        // erase ALL except last two overlapped blocks from freeRegions
        // erase these 2 smaller overlaps
        // insert memRegion to freeRegions

        unsafeFreeImpl(memRegion); // temporary
    }

    bool unsafeFreeImpl(const MemRegion<AddrT> &memRegion) override {
        try {
            auto& regions = freeRegions.at(memRegion.memType);
            regions.insert(memRegion);
            return true;
        } catch(const std::out_of_range &e) {
            return false;
        }
    }

};


}
}
