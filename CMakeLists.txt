cmake_minimum_required (VERSION 3.2)
project(hetarch-proj)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


### LLVM CONFIG ###

if(${MY_TARGET} MATCHES X86)
    set(TARGET_BITWIDTH 64)
elseif(${MY_TARGET} MATCHES ARM)
    set(TARGET_BITWIDTH 32)
elseif(${MY_TARGET} MATCHES STM32)
    set(MY_TARGET ARM)
    set(TARGET_BITWIDTH 32)
    add_definitions(-DHT_ENABLE_STM32)
elseif(${MY_TARGET} MATCHES MSP430)
    set(TARGET_BITWIDTH 16)
else()
    message(FATAL_ERROR "Unknown Target!")
endif()

if(MY_HOST MATCHES ARM)
    set(CMAKE_CROSSCOMPILING True)
    set(TARGET_TRIPLE arm-linux-gnueabihf)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_SYSTEM_NAME Linux)

    set(BIN_DIR /usr/bin)
    set(GCC_PREFIX ${TARGET_TRIPLE}-)
    set(CMAKE_C_COMPILER ${BIN_DIR}/${GCC_PREFIX}gcc)
    set(CMAKE_CXX_COMPILER ${BIN_DIR}/${GCC_PREFIX}g++)
#    set(CMAKE_ASM_COMPILER ${BIN_DIR}/${GCC_PREFIX}as)

    # Compiler target options
    set(SYSROOT /usr/${TARGET_TRIPLE})
    set(GCC_FLAGS "-march=armv7-a -mfloat-abi=hard -mfpu=neon-vfpv4 --sysroot=${SYSROOT}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCC_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_FLAGS}")

    # asio & boost
    include_directories(libs/include)
    # build/link gtest
    if(NOT CMAKE_BUILD_TYPE MATCHES Release)
        # todo: move to tests/CMakeLists.txt?
        set(GTEST_SRC ${CMAKE_CURRENT_SOURCE_DIR}/../googletest/googletest)
        set(GTEST_BIN ${CMAKE_CURRENT_SOURCE_DIR}/../googletest/googletest/googletest-build)
        add_subdirectory(${GTEST_SRC} ${GTEST_BIN} EXCLUDE_FROM_ALL)
    endif()
else()
endif()

# Set needed llvm libs
set(LLVM_COMPONENTS_BASE
        support core irreader
        linker
        passes
        )
set(LLVM_COMPONENTS_TARGET ${MY_TARGET})
set(LLVM_COMPONENTS_HOST ${MY_HOST})

add_definitions(-DHT_ENABLE_${MY_TARGET} -DHT_ENABLE_${MY_HOST} -DTARGET=${MY_TARGET})
add_definitions(-DHETARCH_TARGET_ADDRT=uint${TARGET_BITWIDTH}_t ${MY_PLAFORM_SPECIFIC})
message(STATUS "Building for Host=${MY_HOST}; for Target=${MY_TARGET} (bitwidth: ${TARGET_BITWIDTH})")


# Find LLVM

if(CMAKE_BUILD_TYPE MATCHES Release)
#    find_package(LLVM 5.0 REQUIRED)
else()
    add_definitions(-DIS_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
    #set(TARGET_TRIPLE x86_64-unknown-linux-gnu)
endif()


## Adjust for your use case -- provide LLVM for chosen architecture

set(BASE_DIR /home/hades/projects)
#set(LLVM_DIR ${BASE_DIR}/llvm/${MY_LLVM_BUILD_DIR_NAME}/share/llvm/cmake)
#find_package(LLVM 3.7 REQUIRED PATHS ${LLVM_DIR} NO_DEFAULT_PATH CONFIG)
# By default TARGET_TRIPLE is an empty string
set(LLVM_INSTALL_DIR ${BASE_DIR}/llvms/llvm_50/install_dir_${TARGET_TRIPLE}_${CMAKE_BUILD_TYPE})


set(LLVM_DIR ${LLVM_INSTALL_DIR}/lib/cmake/llvm)
find_package(LLVM 5.0 REQUIRED PATHS ${LLVM_DIR} NO_DEFAULT_PATH CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Get libnames for linking
llvm_map_components_to_libnames(llvm_libs_base ${LLVM_COMPONENTS_BASE})
llvm_map_components_to_libnames(llvm_libs_host ${LLVM_COMPONENTS_HOST})
llvm_map_components_to_libnames(llvm_libs_target ${LLVM_COMPONENTS_TARGET})
#llvm_map_components_to_libnames(llvm_libs_more ${LLVM_COMPONENTS_TARGET} ${LLVM_COMPONENTS_HOST})
set(llvm_libs_more "${llvm_libs_host};${llvm_libs_target}")
#string(REPLACE "x86" "X86" llvm_libs_more ${llvm_libs_more}) # some llvm inconsistency?

set(llvm_libs_all "${llvm_libs_base};${llvm_libs_more}")
message(STATUS "llvm_libs_base: ${llvm_libs_base}")
#message(STATUS "llvm_libs_target: ${llvm_libs_target}")
message(STATUS "llvm_libs_more: ${llvm_libs_more}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

### END LLVM CONFIG ###

# todo: what is it for? it's from old system.
#execute_process(COMMAND "${CMAKE_SOURCE_DIR}/compileMemMapDef.sh" ${CMAKE_SOURCE_DIR}/examples/memmap.def "${MY_ADDR_BITS}"
#        RESULT_VARIABLE cmd_result)
#message(STATUS ${CMAKE_SOURCE_DIR}/compileMemMapDef.sh " " ${cmd_result})

find_package(Threads)


## Set some compiler flags & change linker

message(STATUS "using compiler: ${CMAKE_CXX_COMPILER_ID}")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(LD_OPT "-fuse-ld=lld")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(LD_OPT "-fuse-ld=gold")
endif()

# need -fpermissive to allow casts which lose precision
# need -fno-rtti because of LLVM compiled with -D LLVM_ENABLE_RTTI=OFF
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive ${LD_OPT}")
#set(BASE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fuse-ld=${MY_LD}")
#set(CMAKE_CXX_FLAGS "${BASE_CXX_FLAGS} -fno-rtti")


include_directories(include)

add_library(hetarch STATIC)
target_sources(hetarch
        PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/conn/ht_proto.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/conn/TCPConnImpl.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib/CodeGen.cpp"

        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/types_map.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/dsl_base.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/to_dsl_trait.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/dsl_type_traits.h"

        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/IRTranslator.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/ResidentObjCode.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/ResidentGlobal.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/dsl_meta.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/fun.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/sequence.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/if_else.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/loops.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/op.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/var.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/array.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/ptr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/struct.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/value.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/cast.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl/logging.h"

        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/conn_utils.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/ConnImplBase.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/ht_proto.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/IConnection.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/PosixConn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/SerialConnImpl.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/conn/TCPConnImpl.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/addr_typedef.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/CodeGen.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/CodeLoader.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/Executor.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/MemManager.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/MemResident.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/Pipeline.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/supportingClasses.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/util_type_traits.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/utils.h"

        INTERFACE
        "${CMAKE_CURRENT_SOURCE_DIR}/include/dsl.h"
        )
target_link_libraries(hetarch ${llvm_libs_all})
target_compile_options(hetarch PUBLIC "-fno-rtti")


add_subdirectory(examples)
add_subdirectory(tests)
