#pragma once

#include <type_traits>

#define FUSION_MAX_MAP_SIZE 20
#include <boost/fusion/container/map.hpp>
#include <boost/fusion/include/at_key.hpp>

#include <llvm/IR/Type.h>

namespace hetarch {
namespace utils {


namespace { // hide these using from users of this header
    using boost::fusion::pair;
    using boost::fusion::make_pair;
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
        pair<int64_t*, llvm::PointerType*>
>;

using boost::fusion::at_key;

map_t get_map(llvm::LLVMContext &C) {
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
            make_pair<int64_t*>(llvm::Type::getInt64PtrTy(C))
//            make_pair<>(llvm::Type::get(C)),
    };
};


}
}
