
#get_source_file_property(TMP_FLAGS localLoader.cpp COMPILE_FLAGS)
#STRING(REPLACE "/M-fno-rtti" "" TMP_FLAGS ${TMP_FLAGS})
#set_source_files_properties(localLoader.cpp COMPILE_FLAGS ${TMP_FLAGS})
add_executable(localLoader localLoader.cpp
        ../lib/conn/TCPConnImpl.cpp
        ../lib/conn/ht_proto.c)
target_link_libraries(localLoader ${CMAKE_THREAD_LIBS_INIT})
#set_target_properties(localLoader.cpp PROPERTIES COMPILE_FLAGS ${BASE_CXX_FLAGS} )
