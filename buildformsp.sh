#!/bin/bash

# don't forget to source ARM compiler environment
source /opt/trik-sdk/environment-setup-arm926ejste-oe-linux-gnueabi

BUILD_DIR=`pwd`/ARM-cmake-build-release

echo BUILD_DIR : $BUILD_DIR

mkdir -p $BUILD_DIR
#rm -r $BUILD_DIR/*

cd $BUILD_DIR

pwd

cmake -DCMAKE_BUILD_TYPE=Release ..

#make -j5 triksample1
make -j5 trikSorts1

TRIK_IP=192.168.43.237

#scp libhetarchcg.so root@$TRIK_IP:/home/root/libhetarchcg.so
#scp triksample1 root@$TRIK_IP:/home/root/triksample1
#scp triksample2 root@$TRIK_IP:/home/root/triksample2
scp trikSorts1 root@$TRIK_IP:/home/root/trikSorts1


