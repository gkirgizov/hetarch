#include <stdint.h>

namespace hetarch {
namespace dsl {

template<typename T> int isPrimitiveType() {
    return (std::is_same<T, int8_t>::value ||
            std::is_same<T, uint8_t>::value ||
            std::is_same<T, int16_t>::value ||
            std::is_same<T, uint16_t>::value ||
            std::is_same<T, int32_t>::value ||
            std::is_same<T, uint32_t>::value ||
            std::is_same<T, int64_t>::value ||
            std::is_same<T, uint64_t>::value ||
            std::is_same<T, bool>::value);
}

template<typename T> struct sizeof_safe_t {
    static int value() { return T(nullptr).structSize(); }
};

template<typename T> struct sizeof_safe_t<T*> {
    static int value() { return sizeof(HETARCH_TARGET_ADDRT); }
};

template<typename T, int N> struct sizeof_safe_t<T[N]> {
    static int value() { return N * sizeof_safe_t<T>::value(); }
};

template<> struct sizeof_safe_t<void> { static int value() { throw "unimplemented"; return 0; } };
template<> struct sizeof_safe_t<int8_t> { static int value() { return 1; } };
template<> struct sizeof_safe_t<uint8_t> { static int value() { return 1; } };
template<> struct sizeof_safe_t<bool> { static int value() { return 1; } };
template<> struct sizeof_safe_t<int16_t> { static int value() { return 2; } };
template<> struct sizeof_safe_t<uint16_t> { static int value() { return 2; } };
template<> struct sizeof_safe_t<int32_t> { static int value() { return 4; } };
template<> struct sizeof_safe_t<uint32_t> { static int value() { return 4; } };
template<> struct sizeof_safe_t<int64_t> { static int value() { return 8; } };
template<> struct sizeof_safe_t<uint64_t> { static int value() { return 8; } };

template<typename T> int sizeof_safe() {
    return sizeof_safe_t<T>::value();
}


template<typename T> struct sizeof_safe_bits_t {
    static int value() { return T(nullptr).structSize() * 8; }
};

template<typename T> struct sizeof_safe_bits_t<T*> {
    static int value() { return sizeof(HETARCH_TARGET_ADDRT) * 8; }
};

template<typename T, int N> struct sizeof_safe_bits_t<T[N]> {
    static int value() { return N * sizeof_safe_bits_t<T>::value(); }
};

template<> struct sizeof_safe_bits_t<void> { static int value() { throw "unimplemented"; return 0; } };
template<> struct sizeof_safe_bits_t<bool> { static int value() { return 1; } };
template<> struct sizeof_safe_bits_t<int8_t> { static int value() { return 8; } };
template<> struct sizeof_safe_bits_t<uint8_t> { static int value() { return 8; } };
template<> struct sizeof_safe_bits_t<int16_t> { static int value() { return 16; } };
template<> struct sizeof_safe_bits_t<uint16_t> { static int value() { return 16; } };
template<> struct sizeof_safe_bits_t<int32_t> { static int value() { return 32; } };
template<> struct sizeof_safe_bits_t<uint32_t> { static int value() { return 32; } };
template<> struct sizeof_safe_bits_t<int64_t> { static int value() { return 64; } };
template<> struct sizeof_safe_bits_t<uint64_t> { static int value() { return 64; } };



template<typename T> int sizeof_safe_bits() {
    return sizeof_safe_bits_t<T>::value();
}

template<typename T> std::string tsize() {
    return std::to_string(sizeof_safe<T>() * 8);
}

template<typename T> struct irtypename_t;

template<typename T> struct irtypename_t<T*> {
    static std::string value() {
        return irtypename_t<T>::value() + "*";
    }
};

template<typename T> struct irtypename_t {
    static std::string value() {
        if (std::is_same<T, void>::value) { return "void"; }
        if (std::is_same<T, bool>::value) { return "i1"; }
        return "i" + tsize<T>();
    }
};

template<typename T> std::string IRTypename() {
    return irtypename_t<T>::value();
}

class FunctionAddrNotFixedException: public std::exception {
    virtual const char* what() const throw()
    {
        return "FunctionAddrNotFixedException";
    }
};

inline std::string trimTrailingComma(const std::string &src) {
    auto s = src;
    while (s.length() > 0 && *s.rbegin() == ' ') { s = s.substr(0, s.length() - 1); }
    while (s.length() > 0 && *s.rbegin() == ',') { s = s.substr(0, s.length() - 1); }
    return s;
}

}
}

