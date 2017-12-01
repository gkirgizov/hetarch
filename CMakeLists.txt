cmake_minimum_required (VERSION 2.8.11)
project(hetarch-proj)

set(CMAKE_CXX_STANDARD 11)

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

set(BASE_DIR /home/hades/projects)
set(LLVM_DIR ${BASE_DIR}/llvm/${MY_LLVM_BUILD_DIR_NAME}/share/llvm/cmake)
find_package(LLVM 3.7 PATHS ${LLVM_DIR} NO_DEFAULT_PATH REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package(Threads)

execute_process(COMMAND "${CMAKE_SOURCE_DIR}/compileMemMapDef.sh" ${CMAKE_SOURCE_DIR}/examples/memmap.def "${MY_ADDR_BITS}"
        RESULT_VARIABLE cmd_result)
#message(STATUS ${CMAKE_SOURCE_DIR}/compileMemMapDef.sh " " ${cmd_result})

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
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

add_subdirectory(examples)
