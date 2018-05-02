#pragma once

#include <string_view>

#include "util_type_traits.h"


// from here: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0426r1.html
// Simple constexpr char trait
template <class T>
struct ce_char_trait : std::char_traits<T> {
    using base_t = std::char_traits<T>;
    using size_t = std::size_t;
    using char_type = typename base_t::char_type;

    static constexpr int compare(const char_type* s1, const char_type* s2, size_t n) noexcept {
        for (size_t i = 0; i < n; ++i) {
            if (!base_t::eq(s1[i], s2[i])){
                return base_t::eq(s1[i], s2[i]) ? -1 : 1;
            }
        }

        return 0;
    }

    static constexpr std::size_t length(const char_type* s) noexcept {
        const char_type* const a = s;
        while (!base_t::eq(*s, char_type{})) {
            ++s;
        }

        return s - a;
    }

    static constexpr const char_type* find(const char_type* s, size_t n, const char_type& a) noexcept {
        const char_type* const end = s + n;
        for (; s != end; ++s) {
            if (base_t::eq(*s, a)){
                return s;
            }
        }

        return nullptr;
    }
};

template<typename CharT, typename Traits = std::char_traits<CharT>>
inline constexpr auto make_bsv(const CharT* s) noexcept {
    return std::basic_string_view{s, ce_char_trait<CharT>::length(s)};
}

//using make_sv = make_bsv<char>;
//inline constexpr auto make_sv(const char* s) noexcept {
//    return std::string_view{s, ce_char_trait::length(s)};
//}


namespace hetarch {
namespace utils {


using CharT = unsigned char;

/*template<typename T, typename = typename std::enable_if_t< std::is_standard_layout_v<T> >>
inline const CharT* toBytes(const T &x) { return reinterpret_cast<const CharT*>(&x); }*/

template<typename T, typename = typename std::enable_if_t< std::is_standard_layout_v<T> >>
inline std::size_t toBytes(const T &x, CharT* buffer) {
    memcpy(buffer, reinterpret_cast<const CharT*>(&x), sizeof(T));
    return sizeof(T);
}

// std::tuple is not standart_layout, hence special handling
template<typename ...Ts>
inline std::size_t toBytes(const std::tuple<Ts...> &x, CharT* buffer) {
    std::size_t offset = 0;

    // std::apply to unpack the tuple
    std::apply([&](const auto&... val){
        // it is recursive procedure for case of nested aggregate types
        // dummy array is to use parameter pack expansion through initiliazer list
        std::array dummy = {(
                offset += toBytes(val, buffer + offset)
        )... }; // expand parameter pack in initializer list
    }, x); // unpack tuple

    // total used size (sizeof(tuple)) is different from sum of sizes of constituent types, that 'offset' value holds
    //  it's for consistency with 'fromBytes' procedure (see for more details)
//    return offset;
    return sizeof(std::tuple<Ts...>);
}

template<typename T, typename = typename std::enable_if_t< std::is_standard_layout_v<T> >>
inline T fromBytes(const CharT* bytes) {
    return *reinterpret_cast<const T*>(bytes);
//    static_assert(T::assert_failure && "Cannot deserialize to this type from bytes!");
}

// forward declaration
template<typename Tuple, std::size_t ...I>
static Tuple tupleFromBytesHelper(const CharT* bytes, std::index_sequence<I...>);

// Helper function to work with fromBytes<std::tuple<Ts...>>
//template<typename Tuple, typename = typename std::enable_if_t< is_std_tuple_v<Tuple>, std::true_type >>
//inline Tuple fromBytes(const CharT* bytes) {
template<typename Tuple>
inline std::enable_if_t< is_std_tuple_v<Tuple>, Tuple > fromBytes(const CharT* bytes) {
    return tupleFromBytesHelper<Tuple>( bytes, std::make_index_sequence< std::tuple_size_v<Tuple> >{} );
};


// std::tuple is not standart_layout, hence special handling
// values are assumed to be packed in bytes in strict order as <Ts...>
template<typename ...Ts>
static std::tuple<Ts...> tupleFromBytes(const CharT* bytes) {
    std::size_t offset = 0;
    auto get_offset = [&](std::size_t size) {
        auto old_offset = offset;
        offset += size;
        return old_offset;
    };
    // note: if Ts includes tuple, then actually less bytes will be read than sizeof(tuple)
    //  because tuple is not standart_layout type
    // Although it's possible to avoid this waste of memory
    //  if use own 'sizeof' that will go 'inside' the non-standard_layout types (i.e. tuple)
    return { fromBytes<Ts>( bytes + get_offset(sizeof(Ts)) )... };
}

template<typename Tuple, std::size_t ...I>
static inline Tuple tupleFromBytesHelper(const CharT* bytes, std::index_sequence<I...>) {
    return tupleFromBytes< std::tuple_element_t<I, Tuple>... >(bytes);
}


template<typename F, typename Tuple>
auto for_each(F&& f, Tuple&& t) {
    std::apply([&](auto&& ...item){
        std::make_tuple(
                0, ( std::forward<F>(f)(std::forward<decltype(item)>(item)) , 0)...
//                0, ( f(std::forward<decltype(item)>(item)) , 0)...
        );
    }, std::forward<Tuple>(t));
};


template<typename T>
constexpr std::string_view type_name() {
    // https://stackoverflow.com/a/20170989
    using namespace std;
#ifdef __clang__
    string_view p = __PRETTY_FUNCTION__;
    return string_view(p.data() + 50, p.size() - 50 - 1); // for clang 5, supposedly?
//    return string_view(p.data() + 34, p.size() - 34 - 1); // original
#elif defined(__GNUC__)
    string_view p = __PRETTY_FUNCTION__;
#  if __cplusplus < 201402
    return string_view(p.data() + 36, p.size() - 36 - 1);
#  else
    return string_view(p.data() + 65, p.find(';', 65) - 65);
//    return string_view(p.data() + 49, p.find(';', 49) - 49); // original
#  endif
#elif defined(_MSC_VER)
    string_view p = __FUNCSIG__;
    return string_view(p.data() + 84, p.size() - 84 - 7);
#endif
}

}
}

#define PR_CERR_VAL_TY(VAL) ;std::cerr << hetarch::utils::type_name<decltype( (VAL) )>() << std::endl;
#define PR_CERR_TY(TY) ;std::cerr << hetarch::utils::type_name< TY >() << std::endl;

#ifdef IS_DEBUG
#   define PR_DBG(x) do { std::cerr << x << std::endl; } while (false);
#else
#   define PR_DBG(x)
#endif

namespace hetarch {
namespace utils {
#ifdef IS_DEBUG
constexpr inline bool is_debug = true;
#else
constexpr inline bool is_debug = false;
#endif
}
}


