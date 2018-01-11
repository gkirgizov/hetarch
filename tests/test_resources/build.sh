#!/usr/bin/sh

clang++ -fPIC -emit-llvm self_sufficient.cpp -S -o self_sufficient.ll
clang++ -fPIC -emit-llvm main.cpp -S -o main.ll
clang++ -fPIC -emit-llvm part1.cpp -S -o part1.ll
clang++ -fPIC -emit-llvm part2.cpp -S -o part2.ll

llvm-link part2.ll part1.ll -S -o parts.ll
llvm-link part2.ll part1.ll main.ll -S -o partsm.ll
clang++ partsm.ll -o partsm

