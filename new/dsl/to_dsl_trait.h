#pragma once

#include "var.h"
#include "ptr.h"
#include "array.h"


namespace hetarch {
namespace dsl {


// Type traits for mapping C++ types to DSL type system

template< typename T, typename Enable = void >
struct to_dsl { using type = void; };

template< typename T > using to_dsl_t = typename to_dsl<T>::type;

template< typename T > struct to_dsl<T, typename std::enable_if_t<
        std::is_void_v<T> >>
{ using type = VoidExpr; };

template< typename T > struct to_dsl<T, typename std::enable_if_t<
        std::is_arithmetic_v<T> >>
{ using type = Var<T>; };

template< typename T > struct to_dsl<T, typename std::enable_if_t<
        std::is_pointer_v<T> >>
{
    using type = RawPtr< to_dsl_t< std::remove_pointer_t<T> >
                       , std::is_const_v<T>
    >;
};

template< typename T > struct to_dsl<T, typename std::enable_if_t<
        is_std_array_v<T> >>
{
    using type = Array< to_dsl_t< typename T::value_type >
                      , std::tuple_size_v<T>
                      , std::is_const_v<T>
    >;
};


// Map arbitrary DSL type to DSL Value type
template<typename TdExpr> using param_for_arg_t = typename to_dsl< f_t<TdExpr> >::type;

}
}
