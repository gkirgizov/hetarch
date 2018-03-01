

llvm_map_components_to_libnames(llvm_libs2
        support core irreader
        linker
#        x86  # doesn't work!!! searches for -lLLVMx86 whereas we need -lLLVMX86
        option scalaropts target mc transformutils
        )

set(llvm_target_libs
        LLVMX86Disassembler
        LLVMX86AsmParser
        LLVMX86AsmPrinter
        LLVMX86CodeGen
        )

add_definitions(-DTESTS_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_resources/")

add_executable(gtests main.cpp
        test_utils.cpp
        dsl_tests.cpp
        scratch_tests.cpp
#        sfinae_tests.cpp
        )
target_link_libraries(gtests gtest ${llvm_libs2} ${llvm_target_libs})

add_executable(gtests2 main.cpp
        ../new/TCPConnection.cpp
        ../new/CodeGen.cpp
        test_utils.cpp
        mocks.h
        mem_tests.cpp
        codegen_tests.cpp
#        sfinae_tests.cpp
        )
target_link_libraries(gtests2 gtest ${llvm_libs2} ${llvm_target_libs})
