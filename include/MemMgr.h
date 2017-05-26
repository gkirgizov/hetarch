#ifndef HETARCH_MEMMGR_H
#define HETARCH_MEMMGR_H

#include <vector>
#include <set>
#include <exception>
#include <algorithm>
#include <string>

namespace hetarch {
namespace mem {

class OutOfMemoryException: public std::exception {
    virtual const char* what() const throw()
    {
        return "OutOfMemoryException";
    }
};


namespace MemClass {
    enum MemClass {
        Any,
        ReadWrite,
        ReadPreferred,
    };
}

template<class AddrT>
class MemArea {
public:
    MemArea(AddrT begin, AddrT end, MemClass::MemClass memClass):
            _begin(begin), _end(end), _memClass(memClass) {}
    MemArea(const MemArea &src): _begin(src._begin), _end(src._end), _memClass(src._memClass) {}

    AddrT Size() const { return _end - _begin; }
    AddrT Begin() const { return _begin; }
    AddrT Begin(AddrT newValue) { return _begin = newValue; }
    AddrT End() const { return _end; }
    AddrT End(AddrT newValue) const { return _end = newValue; }
    MemClass::MemClass MemClass() const { return _memClass; }

    void Dump() {
        return "MemArea<uint" + std::to_string(sizeof(AddrT) * 8)
                + "_t>(" + std::to_string(_begin) + ", "
                + std::to_string(_end) + ")";
    }
private:
    AddrT _begin, _end;
    MemClass::MemClass _memClass;
};

template<class AddrT>
class MemMgr {
    std::vector<MemArea<AddrT> > areas;
public:
    MemMgr() {}

    void DefineArea(AddrT begin, AddrT end, MemClass::MemClass _memClass) {
        DefineArea(MemArea<AddrT>(begin, end, _memClass));
    }
    void DefineArea(const MemArea<AddrT> &area) {
        areas.push_back(area);

        // TODO replace sort with insert
        std::sort(std::begin(areas), std::end(areas), [](const MemArea<AddrT> &x, const MemArea<AddrT> &y) {
            return x.Begin() < y.Begin();
        });
    }

    std::pair<AddrT, MemClass::MemClass> Alloc(AddrT size, std::vector<MemClass::MemClass> memClasses) {
        // TODO memory check layout
        std::pair<AddrT, MemClass::MemClass> result;
        bool found = false;
        for (auto mcl: memClasses) {
            for (auto it = areas.begin(); it != areas.end(); it++) {
                if ((mcl == MemClass::Any || it->MemClass() == mcl) && it->Size() >= size)
                {
                    result = std::make_pair(it->Begin(), it->MemClass());
                    if (it->Begin() + size < it->End()) {
                        it->Begin(it->Begin() + size);
                    }
                    else {
                        areas.erase(it);
                    }
                    found = true;
                    break;
                }
            }
            if (found) { break; }
        }
        if (!found) {
            throw OutOfMemoryException();
        }
        return result;
    }
};

}
}

#endif