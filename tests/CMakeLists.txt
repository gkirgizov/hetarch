

add_executable(mem_test scratch_tests.cpp ../new/MemoryManager.h)
#target_include_directories(mem_test "../new")
target_link_libraries(mem_test ${CMAKE_THREAD_LIBS_INIT})


add_executable(scratch_tests scratch_tests.cpp)
target_link_libraries(scratch_tests gtest)


llvm_map_components_to_libnames(llvm_libs2
        support core irreader linker
#        x86  # doesn't work!!! searches for -lLLVMx86 whereas we need -lLLVMX86
        )
#llvm_map_components_to_libnames(llvm_libs2 support core irreader
#        option scalaropts target mc transformutils)

set(llvm_target_libs
        LLVMX86Disassembler
        LLVMX86AsmParser
        LLVMX86AsmPrinter
        LLVMX86CodeGen
        )

add_definitions(-DTESTS_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_resources/")
add_executable(gtests main.cpp
        utils.cpp
        mocks.h)
target_link_libraries(gtests gtest ${llvm_libs2} ${llvm_target_libs})

