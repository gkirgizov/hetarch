<!-- # hetarch -->
# Heterogeneous Embedded Architectures Programming Library

* Paper (in english, SYRCoSE 2018): http://syrcose.ispras.ru/?q=node/13
* Bachelor thesis (in english, contains a bit more information than in the paper): see here http://se.math.spbu.ru/SE/diploma/2018/index by title "Библиотека программирования гетерогенных встраиваемых архитектур" ("Programming Library for Heterogeneous Embedded Architectures" in english)

### Build Instructions
1. Build LLVM (tested with version 5.0) with needed backends (e.g. x86 & arm).
    * example for native build: scripts/build_llvm_test.sh
    * example for cross-compilation build: scripts/build_llvm_cross.sh
2. In CMakeLists.txt setup paths to LLVM; Boost is also required.
3. Provide mandatory defintions to cmake: MY_TARGET, MY_HOST (either modifying CMakeLists or through command line). Possible options: X86 & ARM. It determines the bitwidth of the targets (required in the code) and the set of linked LLVM libs.
4. For cross-compilation to different host: setup compilation options (toolchain, platform architecture etc.) in CMakeLists.
5. Build 'hetarch' target and use it.

### Testing Instructions
* Googletest is used for tests.
* For cross-compilation gtest is included as CMake project; paths must be setup in the main CMakeLists.

Test targets are defined in tests/CMakeLists.txt:
* dsl_tests tests only that DSL constructs are translated to LLVM IR correctly;
* codegen_tests tests the compilation facilities (CodeGenTest suite) and the whole system (CodeLoaderTest suite) where it requires some connected target (for example, running localLoader on the same host).

For details on how to run the tests and generally how to use the system see tests/codegen_tests.cpp file.

#### Targets:
* localLoader cmake target in examples directory is a runtime for Linux targets (e.g. x86 or arm); it requires asio (non-boost version).
* runtime for stm32f429i-discovery microcontroller (armv7e-m) can be found in https://github.com/gkirgizov/hetarch_stm32f4

