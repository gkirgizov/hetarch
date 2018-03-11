#set(llvm_libs_target_x86
#        LLVMX86Disassembler
#        LLVMX86AsmParser
#        LLVMX86AsmPrinter
#        LLVMX86CodeGen
#        LLVMX86Info
#        )
#set(llvm_libs_target_arm
#        LLVMARMDisassembler
#        LLVMARMAsmParser
#        LLVMARMAsmPrinter
#        LLVMARMCodeGen
#        LLVMARMInfo
#        )


add_definitions(-DTESTS_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_resources/")

add_executable(gtests main.cpp
        test_utils.cpp
        dsl_tests.cpp
        )
target_link_libraries(gtests gtest ${llvm_libs_base})
target_compile_options(gtests PUBLIC "-fno-rtti")

add_executable(gtests2 main.cpp
        ../new/conn_utils.cpp
        ../new/CodeGen.cpp
        test_utils.cpp
        mocks.h
        mem_tests.cpp
        codegen_tests.cpp
#        sfinae_tests.cpp
        )
target_link_libraries(gtests2 gtest ${llvm_libs_base} ${llvm_libs_more})
target_compile_options(gtests2 PUBLIC "-fno-rtti")

add_executable(scratch main.cpp
        test_utils.cpp
        scratch_tests.cpp
#        sfinae_tests.cpp
        scratch2.cpp
        )
target_link_libraries(scratch gtest ${llvm_libs_base})
target_compile_options(scratch PUBLIC "-fno-rtti")
