#!/bin/sh

HOST_BIN=/usr/bin
TARGET_TRIPLE=arm-linux-gnueabihf
SYSROOT=/usr/${TARGET_TRIPLE}

LLVM_REL=50
LLVMS_DIR=/home/hades/projects/llvms
BASE_DIR=${LLVMS_DIR}/llvm_${LLVM_REL}
#BASE_DIR=..

SRC_DIR=${BASE_DIR}/llvm
BUILD_TYPE=Release
DIRS_SUFFIX=${TARGET_TRIPLE}
BUILD_DIR=${BASE_DIR}/cmake_build_${DIRS_SUFFIX}_${BUILD_TYPE}
INSTALL_DIR=${BASE_DIR}/install_dir_${DIRS_SUFFIX}_${BUILD_TYPE}
#INSTALL_DIR=$BASE_DIR/install_dir

# Note: need llvm-tblgen of the same version!
LLVM_TABLEGEN=${BASE_DIR}/install_dir__Debug/bin/llvm-tblgen

mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR
cd $BUILD_DIR
echo "Building LLVM from dir: ${SRC_DIR}"
echo "pwd: `pwd`"
echo "build dir: ${BUILD_DIR}"
echo "install dir: ${INSTALL_DIR}"


GCC_PREFIX=${TARGET_TRIPLE}-
# Odroid XU4
# conflict!
#-march=armv7-a \
#-mcpu=cortex-a15 \
GCC_FLAGS="\
-march=armv7-a \
-mfloat-abi=hard \
-mfpu=neon-vfpv4 \
--sysroot=${SYSROOT} \
"

#CLANG_FLAGS="-target $TARGET_TRIPLE"

#GENERATOR="Ninja"
GENERATOR="Unix Makefiles"

    #-DLLVM_TABLEGEN="${HOST_BIN}/llvm-tblgen" \
cmake -G "$GENERATOR" ${SRC_DIR} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_CROSSCOMPILING=True \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=arm \
    -DCMAKE_C_COMPILER=${GCC_PREFIX}gcc \
    -DCMAKE_CXX_COMPILER=${GCC_PREFIX}g++ \
    -DCMAKE_ASM_COMPILER=${GCC_PREFIX}as \
    \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
    \
    -DLLVM_TABLEGEN="${LLVM_TABLEGEN}" \
    -DCLANG_TABLEGEN="${HOST_BIN}/clang-tblgen" \
    -DLLVM_TARGETS_TO_BUILD="ARM" \
    -DLLVM_TARGET_ARCH=ARM \
    -DLLVM_DEFAULT_TARGET_TRIPLE=${TARGET_TRIPLE} \
    -DLLVM_BUILD_32_BITS=ON \
    \
    -DLLVM_INCLUDE_TOOLS=OFF \
    -DLLVM_BUILD_TOOLS=OFF \
    -DLLVM_INCLUDE_UTILS=OFF \
    -DLLVM_BUILD_UTILS=OFF \
    -DLLVM_INCLUDE_EXAMPLES=OFF \
    -DLLVM_BUILD_EXAMPLES=OFF \
    -DLLVM_INCLUDE_TESTS=OFF \
    \
    -DCMAKE_CXX_FLAGS="${GCC_FLAGS}" \
    -DCMAKE_C_FLAGS="${GCC_FLAGS}" \

    ### OTHER OPTIONS TO CONSIDER ###

    #-DCMAKE_EXE_LINKER_FLAGS=-static \
    #-DCMAKE_SHARED_LIBRARY_LINK_C_FLAGS="" \
    #-DCMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS="" \

    #-DLLVM_BUILD_32_BITS=ON \
    #-DLLVM_ENABLE_THREADS=OFF \

    #-DLLVM_INCLUDE_TOOLS=OFF \
    #-DLLVM_BUILD_TOOLS=OFF \
    #-DLLVM_INCLUDE_UTILS=OFF \
    #-DLLVM_BUILD_UTILS=OFF \
    #-DLLVM_INCLUDE_TESTS=OFF \
    #-DLLVM_BUILD_TESTS=OFF \
    #-DLLVM_INCLUDE_EXAMPLES=OFF \
    #-DLLVM_BUILD_EXAMPLES=OFF \

# Things found
# https://stackoverflow.com/questions/19419782/exit-c-text0x18-undefined-reference-to-exit-when-using-arm-none-eabi-gcc
# https://stackoverflow.com/questions/10599038/can-i-skip-cmake-compiler-tests-or-avoid-error-unrecognized-option-rdynamic

# Options explained
#-DLLVM_INCLUDE_TOOLS=OFF
#   we don't need to build tools for target machine. there's even no place to install them (because we're on bare-metal)
