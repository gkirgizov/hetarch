cmake_minimum_required (VERSION 3.2)
project(hetarch-proj)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if ("$ENV{ARCH}" STREQUAL "arm")
    # BUILDING FOR TRIK
    set(MY_LLVM_BUILD_DIR_NAME ARM-cmake-build-release)
    set(MY_LLVM_COMPONENTS msp430 msp430asmprinter msp430codegen msp430desc msp430info)
    set(MY_ADDR_BITS 16)
    set(MY_PLAFORM_SPECIFIC "-DHETARCH_MSP430ONLY")

    message(STATUS "BUILDING FOR TRIK ENV($ENV{ARCH}) AddrT:${MY_ADDR_BITS}")
else()
    # BUILDING FOR LOCAL TESTING - X86_64
    set(MY_LLVM_BUILD_DIR_NAME cmake-build-debug)
    set(MY_LLVM_COMPONENTS msp430 msp430asmprinter msp430codegen msp430desc msp430info
            x86 x86asmparser x86asmprinter x86codegen x86desc x86disassembler x86info x86utils)
    set(MY_ADDR_BITS 64)

    message(STATUS "BUILDING FOR LOCAL TESTING ENV($ENV{ARCH}) AddrT:${MY_ADDR_BITS}")
endif()

# Find LLVM
set(BASE_DIR /home/hades/projects)
#set(LLVM_DIR ${BASE_DIR}/llvm/${MY_LLVM_BUILD_DIR_NAME}/share/llvm/cmake)
#find_package(LLVM 3.7 REQUIRED PATHS ${LLVM_DIR} NO_DEFAULT_PATH CONFIG)
#find_package(LLVM 5.0 REQUIRED)
set(LLVM_INSTALL_DIR ${BASE_DIR}/llvms/llvm_50/install_dir)
set(LLVM_DIR ${LLVM_INSTALL_DIR}/lib/cmake/llvm)
find_package(LLVM 5.0 REQUIRED PATHS ${LLVM_DIR} NO_DEFAULT_PATH CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(Threads)

execute_process(COMMAND "${CMAKE_SOURCE_DIR}/compileMemMapDef.sh" ${CMAKE_SOURCE_DIR}/examples/memmap.def "${MY_ADDR_BITS}"
        RESULT_VARIABLE cmd_result)
#message(STATUS ${CMAKE_SOURCE_DIR}/compileMemMapDef.sh " " ${cmd_result})

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti") # why -fno-rtti?
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=gold")
endif()
message(STATUS "using compiler: ${CMAKE_CXX_COMPILER_ID}")
add_definitions(-DHETARCH_TARGET_ADDRT=uint${MY_ADDR_BITS}_t ${MY_PLAFORM_SPECIFIC})


llvm_map_components_to_libnames(llvm_libs1 support core
        irreader option scalaropts target mc ${MY_LLVM_COMPONENTS}
        transformutils)


include_directories(include)

add_library(hetarch SHARED)
target_sources(hetarch
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/base.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/for.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/if.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/logical.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/switch.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/while.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/base.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/include/cgImpl.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cgImpl.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/cg.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dslutils.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/IRDumper.cpp"

#        "${CMAKE_CURRENT_SOURCE_DIR}/include/I2CConnection.h"
#        "${CMAKE_CURRENT_SOURCE_DIR}/include/i2ccommands.h"
#        "${CMAKE_CURRENT_SOURCE_DIR}/lib/tcpTestConn.cpp"
#        "${CMAKE_CURRENT_SOURCE_DIR}/include/TCPTestConn.h"

        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/cg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/IRDumper.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/MemMgr.h"

        INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl.h"
        )
target_link_libraries(hetarch ${llvm_libs1})


add_library(hetarchcg SHARED)
target_sources(hetarchcg
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/include/cgImpl.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/cgImpl.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/lib/cg.cpp"
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/include/cg.h"
        )
target_link_libraries(hetarchcg ${llvm_libs1})


add_library(hetarch_trik)
target_sources(hetarch_trik
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/trikMspI2c.cpp"
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/trikMspI2c.h"
        )


#add_library(hetarch_new_mem)
#target_sources(hetarch_new_mem
#        PRIVATE
#        "${CMAKE_CURRENT_SOURCE_DIR}/new/MemoryManager.cpp"
#        PUBLIC
#        "${CMAKE_CURRENT_SOURCE_DIR}/new/MemoryManager.h"
#        )
#find_package(Boost REQUIRED)
#target_link_libraries(hetarch_new_mem )
#message(STATUS "Found Boost ${Boost_Version}")
#message(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
#target_include_directories(hetarch_new_mem PUBLIC new/ ${Boost_INCLUDE_DIRS})


add_library(temp_target
        tests/test_code/test_code.cpp
        new/CodeGen.h new/MemoryManager.h new/MemResident.h new/dsl/dsl_base.h new/supportingClasses.h new/CodeLoader.h new/IConnection.h new/dsl/sequence.h new/dsl/IRTranslator.h new/utils.h new/dsl/types_map.h new/dsl/if_else.h new/dsl/ResidentObjCode.h new/dsl/op.h new/dsl/var.h new/dsl/fun.h new/dsl/dsl_type_traits.h new/dsl/array.h new/dsl/ptr.h new/dsl/loops.h new/dsl.h)

add_subdirectory(examples)
add_subdirectory(tests)
