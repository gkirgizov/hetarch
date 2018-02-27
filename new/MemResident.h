#pragma once


#include "MemoryManager.h"

namespace hetarch {

template<typename AddrT>
class MemResident {
    mem::MemManager<AddrT>& m_memManager;
    mem::MemRegion<AddrT> m_memRegion;
    bool m_loaded;

public:
    const bool unloadable;

    MemResident(mem::MemManager<AddrT>& memManager, mem::MemRegion<AddrT> memRegion, bool unloadable = false)
            : m_memManager(memManager), m_memRegion(memRegion), m_loaded(true), unloadable(unloadable)
    {}

    bool loaded() const { return m_loaded; }

    void unload() {
        if (unloadable && m_loaded) {
            m_memManager.free(m_memRegion);
            m_loaded = false;
        }
    };

    mem::MemRegion<AddrT> memRegion() const { return m_memRegion; }

    // Move-only type to enforce more reasonable usage
    MemResident(MemResident<AddrT> &&) = default;
    MemResident &operator=(MemResident<AddrT> &&) = default;

};

}
