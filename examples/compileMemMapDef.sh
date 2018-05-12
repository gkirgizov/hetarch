#!/usr/bin/env bash

pwd

mkdir -p geninc


PLATFORM="uint$2_t"

DIR=geninc
CPPFILE=$DIR/memregmap.cpp
INCFILE=$DIR/memregmap.inc

echo "#include <dsl.h>" > $INCFILE
echo "#include <cstdint>" >> $INCFILE
echo "" >> $INCFILE
echo "using hetarch::dsl::Global;" >> $INCFILE
echo "typedef $PLATFORM RegsPlatform;" >> $INCFILE
echo "" >> $INCFILE
while IFS='' read -r line || [[ -n "$line" ]]; do
    ADDR=$(echo $line | cut -d " " -f 1)
    NAME=$(echo $line | cut -d " " -f 2)
    echo "extern Global<RegsPlatform, uint16_t> $NAME;" >> $INCFILE
done < "$1"


echo "#include \"memregmap.inc\"" > $CPPFILE
echo "" >> $CPPFILE
echo "using hetarch::dsl::Global;" >> $CPPFILE
echo "typedef $PLATFORM RegsPlatform;" >> $CPPFILE
echo "" >> $CPPFILE
while IFS='' read -r line || [[ -n "$line" ]]; do
    ADDR=$(echo $line | cut -d " " -f 1)
    NAME=$(echo $line | cut -d " " -f 2)
    echo "Global<RegsPlatform, uint16_t> $NAME(0x$ADDR);" >> $CPPFILE
done < "$1"



