#!/bin/bash

# don't forget to source ARM compiler environment
source /opt/trik-sdk/environment-setup-arm926ejste-oe-linux-gnueabi

unset PYTHONHOME

LLVM_SRC=${LLVM_SRC:-/home/$USER/Documents/GitHub/llvm}
BUILD_DIR=$LLVM_SRC/ARM-cmake-build-release
TABLEGEN=$LLVM_SRC/cmake-build-debug/bin/llvm-tblgen

echo LLVM_SRC  : $LLVM_SRC
echo BUILD_DIR : $BUILD_DIR
echo TABLEGEN  : $TABLEGEN

mkdir -p $BUILD_DIR
#rm -r $BUILD_DIR/*

cd $BUILD_DIR

pwd
cmake -DLLVM_INCLUDE_TESTS=false \
    -DLLVM_DEFAULT_TARGET_TRIPLE=msp430-linux-gnueabihf \
    -DLLVM_TABLEGEN=$TABLEGEN \
    -DLLVM_TARGET_ARCH=MSP430 \
    -DLLVM_TARGETS_TO_BUILD=MSP430 \
    -DPYTHON_EXECUTABLE=/usr/bin/python2.7 \
    -DCMAKE_BUILD_TYPE=Release ..

make -j5 llc

exit 0


-DPYTHON_LIBRARY=/usr/lib/python2.7/config-x86_64-linux-gnu/libpython2.7.so \