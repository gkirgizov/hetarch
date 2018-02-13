#pragma once

#include <type_traits>

#include "dsl_base.h"
#include "../utils.h"


namespace hetarch {
namespace dsl {


template<typename T>
struct remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};
template<typename T> using remove_cvref_t = typename remove_cvref<T>::type;


template<typename Td>
struct inner_type {
//    constexpr auto Td_typename = utils::type_name<remove_cvref_t<Td>>().data();
    static_assert(std::is_base_of_v<DSLBase, remove_cvref_t<Td>>, "provided typename isn't derived from DSLBase");
    using type = typename std::remove_reference_t<Td>::type;
};
// Specisalisation for Arrays
//template<typename T, std::size_t N>
//struct inner_type<std::array<T, N>> { using type = T; };
template<typename Td> using i_t = typename inner_type<Td>::type;


//template<typename Td> using i_t = typename std::remove_reference_t<Td>::type;

template<typename ...Tds>
struct is_dsl : public std::conditional_t<
        (... && std::is_base_of_v<DSLBase, remove_cvref_t<Tds>>),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_dsl_v = is_dsl<Tds...>::value;


template<typename ...Tds>
struct is_ev : public std::conditional_t<
        (... && (std::is_base_of_v<ExprBase, remove_cvref_t<Tds>> || std::is_base_of_v<ValueBase, remove_cvref_t<Tds>>)),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_ev_v = is_ev<Tds...>::value;


//template<typename ...Ts, typename ...Tds, typename = typename std::enable_if_t<sizeof...(Tds) == sizeof...(Ts)>>
//struct is_same_raw : public std::conditional_t<
//        std::is_same_v< std::tuple<i_t<Tds>...>, std::tuple<Ts...> >,
//        std::true_type,
//        std::false_type
//> {};


template<typename T, typename ...Tds>
struct is_same_raw : public std::conditional_t<
        (... && std::is_same_v<T, i_t<Tds>>),
        std::true_type,
        std::false_type
> {};
template<typename T, typename ...Tds> inline constexpr bool is_same_raw_v = is_same_raw<T, Tds...>::value;


template<typename>
struct is_std_array : public std::false_type {};
template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : public std::true_type {};
template<typename Arr>
inline constexpr bool is_std_array_v = is_std_array<Arr>::value;

}
}
