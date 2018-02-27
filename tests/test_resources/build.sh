#!/usr/bin/sh

clang++ -O0 -fPIC -emit-llvm main.cpp -S -o main.ll
clang++ -O0 -fPIC -emit-llvm do_nothing.cpp -S -o do_nothing.ll
clang++ -O0 -fPIC -emit-llvm part1.cpp -S -o part1.ll
clang++ -O0 -fPIC -emit-llvm part2.cpp -S -o part2.ll

llvm-link part2.ll part1.ll -S -o parts.ll
llvm-link part2.ll part1.ll main.ll -S -o partsm.ll
clang++ -O0 partsm.ll -o partsm

clang++ -O0 -fPIC -emit-llvm self_sufficient.cpp -S -o self_sufficient.ll
cp self_sufficient.ll ss.ll
llvm-link main.ll ss.ll -S -o ss.main.ll

#opt -pgo-instr-gen ss.ll -S -o ss.profiled1.ll
#opt -instrprof ss.profiled1.ll -S -o ss.profiled2.ll
opt -pgo-instr-gen ss.main.ll -S -o ss.main.profiled1.ll
opt -instrprof ss.main.profiled1.ll -S -o ss.main.profiled2.ll

llc -relocation-model=pic -O0 -filetype=obj ss.main.profiled2.ll
clang++ -O0 -fprofile-instr-generate ss.main.profiled2.o -o ss.main.profiled2

#clang++ -O0 -fprofile-instr-generate ss.main.ll -S -o ss.main.profiled2.direct.ll

#llc -relocation-model=pic -O0 -filetype=obj ss.main.profiled1.ll

TARGET=msp430
opt -instrprof ss.main.profiled1.ll -S -o ss.main.profiled2.$TARGET.ll -mtriple=$TARGET
#llc -relocation-model=pic -O0 -filetype=obj ss.main.profiled2.$TARGET.ll -mtriple=$TARGET  # target doesn't support generation of hits filetyp
llc -relocation-model=pic -O0 -filetype=asm ss.main.profiled2.$TARGET.ll -mtriple=$TARGET
#llc -relocation-model=pic -O0 -filetype=asm ss.main.profiled1.ll -mtriple=$TARGET  # obviously fails: not-lowered intrinsic present


