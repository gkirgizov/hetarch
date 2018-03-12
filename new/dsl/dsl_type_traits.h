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
    static_assert(std::is_base_of_v<DSLBase, remove_cvref_t<Td>>, "provided typename isn't derived from DSLBase");
    using type = typename std::remove_reference_t<Td>::type;
};
template<typename Td> using f_t = typename inner_type<Td>::type;

template<typename Td>
struct inner_type_stripped {
    using type = typename remove_cvref<f_t<Td>>::type;
};
template<typename Td> using i_t = typename inner_type_stripped<Td>::type;


//template<typename Td> using i_t = typename std::remove_reference_t<Td>::type;


template<typename ...Tds>
struct is_dsl : public std::conditional_t<
        (... && std::is_base_of_v<DSLBase, remove_cvref_t<Tds>>),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_dsl_v = is_dsl<Tds...>::value;


template<typename ...Tds>
struct is_expr : public std::conditional_t<
        (... && std::is_base_of_v<ExprBase, remove_cvref_t<Tds>>),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_expr_v = is_expr<Tds...>::value;


template<typename ...Tds>
struct is_val : public std::conditional_t<
        (... && std::is_base_of_v<ValueBase, remove_cvref_t<Tds>>),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_val_v = is_val<Tds...>::value;


template<typename ...Tds>
struct is_ev : public std::conditional_t<
        (... && (std::is_base_of_v<ExprBase, remove_cvref_t<Tds>> || std::is_base_of_v<ValueBase, remove_cvref_t<Tds>>)),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_ev_v = is_ev<Tds...>::value;


template<typename ...Tds>
struct is_unit : public std::conditional_t<
        (... && std::is_void_v<i_t<Tds>>),
        std::true_type,
        std::false_type
> {};
template<typename ...Tds> inline constexpr bool is_unit_v = is_unit<Tds...>::value;



// Accessors for aggregate types (Ptr, Array, Struct)

template<typename Td> struct get_dsl_element { using type = typename std::remove_reference_t<Td>::dsl_element_t; };
template<typename Td> using get_dsl_element_t = typename get_dsl_element<Td>::type;

template<typename Td> struct get_element { using type = typename std::remove_reference_t<Td>::element_t; };
template<typename Td> using get_element_t = typename get_element<Td>::type;

// Accessor to Base class of Value, which provides dsl operations (e.g. ArrayBase provides operator[])
template< typename Td, typename TdChild
        , typename = typename std::enable_if_t< is_val_v<Td> >>
struct get_base {
    using type = typename std::remove_reference_t<Td>::template base_t<TdChild>;
};
template< typename Td, typename TdChild >
using get_base_t = typename get_base<Td, TdChild>::type;





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

template<typename>
struct is_std_tuple : public std::false_type {};
template<typename ...Ts>
struct is_std_tuple<std::tuple<Ts...>> : public std::true_type {};
template<typename ...Ts>
inline constexpr bool is_std_tuple_v = is_std_tuple<Ts...>::value;





}
}
