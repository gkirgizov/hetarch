cmake_minimum_required (VERSION 2.8.11)
project(hetarch)

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

set(LLVM_DIR /home/user/Documents/GitHub/llvm/${MY_LLVM_BUILD_DIR_NAME}/share/llvm/cmake)

find_package(LLVM 3.7 PATHS /home/user/Documents/GitHub/llvm/${MY_LLVM_BUILD_DIR_NAME}/share/llvm/cmake NO_DEFAULT_PATH REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

find_package (Threads)

execute_process(COMMAND "${CMAKE_SOURCE_DIR}/compileMemMapDef.sh" ${CMAKE_SOURCE_DIR}/examples/memmap.def "${MY_ADDR_BITS}"
        RESULT_VARIABLE cmd_result)
#message(STATUS ${CMAKE_SOURCE_DIR}/compileMemMapDef.sh " " ${cmd_result})

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories("/home/user/Downloads/asio-1.10.6/include")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -DHETARCH_TARGET_ADDRT=uint${MY_ADDR_BITS}_t ${MY_PLAFORM_SPECIFIC}")


add_library(hetarchcg SHARED lib/cgImpl.cpp lib/cg.cpp)

llvm_map_components_to_libnames(llvm_libs1 support core
        irreader option scalaropts target mc ${MY_LLVM_COMPONENTS}
        transformutils)


target_link_libraries(hetarchcg ${llvm_libs1})


add_executable(gensample1 examples/gensample1.cpp lib/tcpTestConn.cpp lib/IRDumper.cpp lib/dsl.cpp)
target_link_libraries(gensample1 hetarchcg)


add_executable(triksample1 examples/triksample1.cpp lib/trikMspI2c.cpp)
target_link_libraries(triksample1 hetarchcg)


add_executable(triksample2 examples/triksample2.cpp
        lib/trikMspI2c.cpp lib/IRDumper.cpp lib/dsl.cpp examples/msp_utils.cpp)
target_link_libraries(triksample2 hetarchcg)
target_include_directories(triksample2 PUBLIC include)

add_executable(trikSorts1 examples/trikSorts1.cpp
        lib/trikMspI2c.cpp lib/IRDumper.cpp lib/dsl.cpp examples/msp_utils.cpp ${CMAKE_BINARY_DIR}/geninc/memregmap.cpp)
target_link_libraries(trikSorts1 hetarchcg)
target_include_directories(trikSorts1 PUBLIC ${CMAKE_BINARY_DIR}/geninc/ include)

add_executable(motorsSample examples/motorsSample.cpp ${CMAKE_BINARY_DIR}/geninc/memregmap.cpp)
target_link_libraries(motorsSample hetarchcg)
target_include_directories(motorsSample PUBLIC ${CMAKE_BINARY_DIR}/geninc/ include)


add_executable(hostSample1 examples/hostSample1.cpp
         lib/IRDumper.cpp lib/dsl.cpp lib/tcpTestConn.cpp)
target_link_libraries(hostSample1 hetarchcg)
target_include_directories(hostSample1 PUBLIC include)


add_executable(localLoader examples/localLoader.cpp lib/tcpTestConn.cpp)
target_link_libraries (localLoader ${CMAKE_THREAD_LIBS_INIT})






