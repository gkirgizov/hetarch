
MACRO(HEADER_DIRS return_list cur_dir)
#    FILE(GLOB_RECURSE new_list FOLLOW_SYMLINKS RELATIVE ${cur_dir} *.h)
    FILE(GLOB_RECURSE new_list FOLLOW_SYMLINKS *.h)
    SET(dir_list "")
    FOREACH(file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        LIST(APPEND dir_list ${dir_path})
    ENDFOREACH()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()


#add_definitions(-DTESTS_RES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_resources/")
# determine relative path for portability of executables using it
#set(TESTS_RES_DIR_ABS ${CMAKE_CURRENT_SOURCE_DIR}/test_resources/)
#file(RELATIVE_PATH TESTS_RES_DIR ${CMAKE_CURRENT_BINARY_DIR} ${TESTS_RES_DIR_ABS})
#add_definitions(-DTESTS_RES_DIR="${TESTS_RES_DIR}/")
#message(STATUS "Path to test_resources dir: ${TESTS_RES_DIR}")

# Create symlink to use local test_resources dir (for better portability)
execute_process(COMMAND ln -s ${CMAKE_CURRENT_SOURCE_DIR}/test_resources WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DTESTS_RES_DIR="./test_resources/")

add_executable(dsl_tests main.cpp
        test_utils.cpp
        dsl_tests.cpp
        )
target_link_libraries(dsl_tests gtest ${llvm_libs_base})
target_compile_options(dsl_tests PUBLIC "-fno-rtti")

add_executable(codegen_tests main.cpp
        test_utils.cpp
        mocks.h
        mem_tests.cpp
        codegen_tests.cpp
        )
target_link_libraries(codegen_tests hetarch gtest ${llvm_libs_base} ${llvm_libs_more})
target_compile_options(codegen_tests PUBLIC "-fno-rtti")


add_executable(scratch main.cpp
        test_utils.cpp
        scratch_tests.cpp
        )
target_link_libraries(scratch gtest ${llvm_libs_base})
target_compile_options(scratch PUBLIC "-fno-rtti")


add_executable(dsl_examples examples/dsl_examples.cpp)
target_link_libraries(dsl_examples ${llvm_libs_base})
target_compile_options(dsl_examples PUBLIC "-fno-rtti")
