#pragma once

#include <string_view>


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

// todo: specialise translating initial value to char* (i.e. for Array, for Struct)
template<typename T>
const char* toBytes(const T &x) {
    if constexpr (std::is_arithmetic_v<T>) {
        return reinterpret_cast<const char*>(&x);
    }
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

#ifdef DEBUG
#   define PR_DBG(x) do { std::cerr << x << std::endl; } while (false);
#else
#   define PR_DBG(x)
#endif

namespace hetarch {
namespace utils {
#ifdef DEBUG
constexpr inline bool is_debug = true;
#else
constexpr inline bool is_debug = false;
#endif
}
}


