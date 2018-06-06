
add_executable(localLoader localLoader.cpp
        ../lib/conn/TCPConnImpl.cpp
        ../lib/conn/ht_proto.c)
## Note: also requires Asio (non-boost version) & cxxopts (command line args parser)
target_link_libraries(localLoader ${CMAKE_THREAD_LIBS_INIT})
#set_target_properties(localLoader.cpp PROPERTIES COMPILE_FLAGS ${BASE_CXX_FLAGS} )
