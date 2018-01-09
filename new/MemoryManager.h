#pragma once


#include <vector>
#include <memory>

#include <unordered_map>
#include <set>


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
    explicit MemRegion() = default;  // for constructing false-y value
    MemRegion(const AddrT start, const AddrT size, const MemType memType)
    : size(size), start(start), end(start+size), memType(memType)
    {}

    const AddrT size;
    const AddrT start;
    const AddrT end;
    const MemType memType;

    explicit operator bool() const { return size == 0; }
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
    virtual MemRegion<AddrT> alloc(AddrT memSize, MemType memType) = 0;

    // to control memory layout manually -- 'register' memory usage
    virtual bool tryAlloc(const MemRegion<AddrT> &memRegion) = 0;

    virtual void free(const MemRegion<AddrT> &memRegion) = 0;

    /// Free without any checks. Can be easily misused. Only for the most confident callers who know definetly what they've allocated.
    /// \param memRegion previously allocated region
    /// \return true if memRegion has consistent MemType
    virtual bool unsafeFree(const MemRegion<AddrT> &memRegion) = 0;

    virtual ~MemManager() = default;

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

}


template<typename AddrT>
class MemManagerSequentialFit: public MemManager<AddrT> {

    using impl_cont_t = std::multiset<MemRegion, mem_mgr_impl::cmp_size_less>;

    std::unordered_map<MemType, impl_cont_t> freeRegions{};

    // todo: coalesce blocks when natural (by-address) ordering
    void insertRegion(MemRegion<AddrT> newRegion, impl_cont_t &regions) {
        regions.insert(newRegion);
    }

public:
    explicit MemManagerSequentialFit(MemRegion<AddrT> availableMem) {
        freeRegions[availableMem.memType].insert(std::move(availableMem));
    }

    explicit MemManagerSequentialFit(const std::vector<MemRegion<AddrT>> &availableMem) {
        for (MemRegion<AddrT> memRegion : availableMem) {
            freeRegions[memRegion.memType].insert(std::move(memRegion));
        }
    }

    MemRegion<AddrT> alloc(AddrT memSize, MemType memType) override {
        // todo: if there's no memory of such type, then fallback to less-restrictive memType
        // don't forget to rewrite memType
        auto regions = freeRegions[memType];

        // todo: not the most efficient; for best-fit, worst-fit etc. can search better than linear
        for (auto it = std::begin(regions); it != std::end(regions); ++it) {
            if (memSize <= it->size) {
                regions.erase(it);
                MemRegion<AddrT> splittedRegion(it->start + memSize, it->size - memSize, memType);
                insertRegion(splittedRegion, regions);
                MemRegion<AddrT> foundRegion(it->start, memSize, memType);
                return foundRegion;
            }
        }
        return MemRegion<AddrT>();
    }

    bool tryAlloc(const MemRegion<AddrT> &memRegion) override {
        auto memType = memRegion.memType;
        auto regions = freeRegions[memType];

        // todo: not the most efficient; for best-fit, worst-fit etc. can fail earlier
        for (auto it = std::begin(regions); it != std::end(regions); ++it) {
            if (memRegion.start >= it->start && memRegion.end <= it->end) {
                regions.erase(it);
                if (memRegion.start > it->start) {
                    regions.insert(MemRegion<AddrT>(it->start, memRegion.start - it->start, memType));
                }
                if (memRegion.end < it->end) {
                    regions.insert(MemRegion<AddrT>(memRegion.end, it->end - memRegion.end));
                }
                return true;
            }
        }
        return false;
    }

    void free(const MemRegion<AddrT> &memRegion) override {
        // find ALL overlapped blocks
        // among them find 2 'border' blocks
        // find overlaps with them
        // split them correspondingly
        // erase ALL except last two overlapped blocks from freeregions
        // erase these 2 smaller overlaps
        // insert memRegion to freeRegions
    }

    bool unsafeFree(const MemRegion<AddrT> &memRegion) override {
        try {
            auto regions = freeRegions.at(memRegion.memType);
            regions.insert(memRegion);
            return true;
        } catch(const std::out_of_range &e) {
            return false;
        }
    }

};


}
}

