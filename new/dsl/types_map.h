#pragma once

#include <cstdint>
#include <type_traits>
#include <array>

#define FUSION_MAX_MAP_SIZE 20
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/include/at_key.hpp>

#include "llvm/IR/Type.h"
#include "llvm/Support/Casting.h"

#include "../dsl/dsl_type_traits.h"


namespace hetarch {
namespace utils {

namespace { // hide these using from users of this header
    using boost::fusion::pair;
    using boost::fusion::make_pair;
    using boost::fusion::at_key;
}

using map_t = boost::fusion::map<
        pair<void, llvm::Type*>,

        pair<float  , llvm::Type*>,
        pair<double , llvm::Type*>,
        pair<bool   , llvm::IntegerType*>,
        pair<int8_t , llvm::IntegerType*>,
        pair<int16_t, llvm::IntegerType*>,
        pair<int32_t, llvm::IntegerType*>,
        pair<int64_t, llvm::IntegerType*>,

        pair<float  *, llvm::PointerType*>,
        pair<double *, llvm::PointerType*>,
        pair<bool   *, llvm::PointerType*>,
        pair<int8_t *, llvm::PointerType*>,
        pair<int16_t*, llvm::PointerType*>,
        pair<int32_t*, llvm::PointerType*>,
        pair<int64_t*, llvm::PointerType*>,

        pair<char   , llvm::IntegerType*>,
        pair<char*  , llvm::PointerType*>
>;



class LLVMMapper {
    const map_t type_map;

    static map_t get_type_map(llvm::LLVMContext &C) {
//    map_t LLVMMapper::get_type_map(llvm::LLVMContext &C) {
        return map_t{
                make_pair<void>(llvm::Type::getVoidTy(C)),

                make_pair<float  >(llvm::Type::getFloatTy(C)),
                make_pair<double >(llvm::Type::getDoubleTy(C)),
                make_pair<bool   >(llvm::Type::getInt1Ty(C)),
                make_pair<int8_t >(llvm::Type::getInt8Ty(C)),
                make_pair<int16_t>(llvm::Type::getInt16Ty(C)),
                make_pair<int32_t>(llvm::Type::getInt32Ty(C)),
                make_pair<int64_t>(llvm::Type::getInt64Ty(C)),

                make_pair<float  *>(llvm::Type::getFloatPtrTy(C)),
                make_pair<double *>(llvm::Type::getDoublePtrTy(C)),
                make_pair<bool   *>(llvm::Type::getInt1PtrTy(C)),
                make_pair<int8_t *>(llvm::Type::getInt8PtrTy(C)),
                make_pair<int16_t*>(llvm::Type::getInt16PtrTy(C)),
                make_pair<int32_t*>(llvm::Type::getInt32PtrTy(C)),
                make_pair<int64_t*>(llvm::Type::getInt64PtrTy(C)),
//            make_pair<>(llvm::Type::get(C)),

                make_pair<char   >(llvm::Type::getInt8Ty(C)),
                make_pair<char  *>(llvm::Type::getInt8PtrTy(C))
        };
    }

    using addr_t = HETARCH_TARGET_ADDRT;
public:
    explicit LLVMMapper(llvm::LLVMContext& C) : type_map{get_type_map(C)} {}

    template<typename Tcv>
    inline auto get_type() {
        using T = std::remove_cv_t<Tcv>;
        if constexpr (std::is_void_v<T> || std::is_same_v<bool, T> || std::is_floating_point_v<T>) {
            return utils::at_key<T>(type_map);
        } else if constexpr (std::is_integral_v<T>) {
            // LLVM make no distinction between signed and unsigned (https://stackoverflow.com/a/14723945)
            return utils::at_key<std::make_signed_t<T>>(type_map);
        } else if constexpr (std::is_reference_v<T>) {
            return get_type<std::remove_reference_t<T>>()->getPointerTo();
        } else if constexpr (std::is_pointer_v<T>) {
            return get_type<std::remove_pointer_t<T>>()->getPointerTo();
        } else if constexpr (dsl::is_std_array_v<T>) {
            return llvm::ArrayType::get(get_type<typename T::value_type>(), std::tuple_size<T>::value);
        } else {
//            static_assert(false, "Unknown type provided!");
            static_assert(T::fail_compile, "Unknown type provided!");
        }
    }

    // multidimensional arrays should be handled
//    template<typename Array, typename = typename std::enable_if_t<std::is_array_v<Array>>>
//    inline llvm::ArrayType* get_type() {
//        std::size_t N = std::extent_v<Array>;
//        using X = std::remove_extent_t<Array>;
//        return llvm::ArrayType::get(get_type<X>(), N);
//    };

    template<typename T, std::size_t N>
    inline llvm::ArrayType* get_type() { return llvm::ArrayType::get(get_type<T>(), N); };

/*    template<typename RetT, typename... Args>
    inline llvm::FunctionType* get_func_type(bool isVarArg = false) {
        return llvm::FunctionType::get(
                get_type<RetT>(), { get_type<Args>()... }, isVarArg
        );
    }*/

    template<typename RetT, typename ArgsTuple
            , typename Inds = std::make_index_sequence<std::tuple_size_v<ArgsTuple>>
            , typename = typename std::enable_if_t<dsl::is_std_tuple_v<ArgsTuple>> >
    inline llvm::FunctionType* get_func_type(bool isVarArg = false) {
        return llvm::FunctionType::get(
                get_type<RetT>(), get_func_args_type<ArgsTuple>(Inds{}), isVarArg
        );
    }
    template<typename ArgsTuple, std::size_t ...I>
    inline std::array<llvm::Type*, sizeof...(I)> get_func_args_type(std::index_sequence<I...>) {
        return { get_type< std::tuple_element_t<I, ArgsTuple> >()... };
    }

    // todo: somehow restrict bit width of arithmetic types depending on target platform
    template<typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    auto get_const(T val) {
        if constexpr (std::is_floating_point_v<T>) {
            return llvm::ConstantFP::get(get_type<T>(), static_cast<double>(val));
        } else if constexpr (std::is_unsigned_v<T>) {
            return llvm::ConstantInt::get(get_type<T>(), static_cast<uint64_t>(val));
        } else {
            static_assert(std::is_signed_v<T>);
            return llvm::ConstantInt::getSigned(get_type<T>(), static_cast<int64_t>(val));
        }
    }

/*    template<typename T, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
    auto get_const(T* val) {
//        auto addr_val = reinterpret_cast<std::size_t>(val);
        auto addr_val = static_cast<addr_t>(reinterpret_cast<std::size_t>(val));
        auto llvm_addr_val = get_const(addr_val);
//        auto ptr_ty = llvm_addr_val->getType()->getPointerTo();
//        return llvm::cast<ptr_ty>(llvm_addr_val);
        return llvm::cast<llvm::PointerType>(llvm_addr_val);
    }*/

    template<typename T, std::size_t N, typename = typename std::enable_if_t<std::is_arithmetic_v<T>>>
//    auto get_const(const T (&arr)[N]) {
    auto get_const(const std::array<T, N> &arr) {
        // todo ensure it works when vals are out of scope after return
        std::array<llvm::Constant*, N> vals;
        for (auto i = 0; i < N; ++i) {
            vals[i] = get_const(arr[i]);
        }
        return llvm::ConstantArray::get(get_type<std::array<T, N>>(), llvm::ArrayRef<llvm::Constant*>{vals});
    };

};

}
}
