#pragma once

#include <type_traits>
#include <tuple>
#include <array>


namespace hetarch {
namespace utils {


template<typename>
struct is_std_array : std::false_type {};
template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};
template<typename Arr>
inline constexpr bool is_std_array_v = is_std_array<Arr>::value;

template<typename>
struct is_std_tuple : std::false_type {};
template<typename ...Ts>
struct is_std_tuple<std::tuple<Ts...>> : std::true_type {};
template<typename ...Ts>
inline constexpr bool is_std_tuple_v = is_std_tuple<Ts...>::value;


}
}
