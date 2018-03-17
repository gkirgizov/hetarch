
#get_source_file_property(TMP_FLAGS localLoader.cpp COMPILE_FLAGS)
#STRING(REPLACE "/M-fno-rtti" "" TMP_FLAGS ${TMP_FLAGS})
#set_source_files_properties(localLoader.cpp COMPILE_FLAGS ${TMP_FLAGS})
add_executable(localLoader localLoader.cpp
        ../new/TCPConnection.cpp ../new/conn_utils.h)
target_link_libraries(localLoader ${CMAKE_THREAD_LIBS_INIT})
#set_target_properties(localLoader.cpp PROPERTIES COMPILE_FLAGS ${BASE_CXX_FLAGS} )


set(TCP_SOURCES ../include/TCPTestConn.h ../lib/tcpTestConn.cpp)
set(I2C_SOURCES ../include/i2ccommands.h ../include/I2CConnection.h
        msp_utils.h msp_utils.cpp)


add_executable(gensample1 gensample1.cpp
        basic.h
        ${TCP_SOURCES})
target_link_libraries(gensample1 hetarch)

add_executable(hostSample1 hostSample1.cpp
        logger.h arithm.h basic.h example_utils.h
        ${TCP_SOURCES})
target_link_libraries(hostSample1 hetarch)


add_executable(triksample1 triksample1.cpp)
target_link_libraries(triksample1 hetarchcg hetarch_trik)

add_executable(triksample2 triksample2.cpp
        ${I2C_SOURCES})
target_link_libraries(triksample2 hetarch hetarch_trik)


add_executable(trikSorts1 trikSorts1.cpp
        msp_ext_points.h sensor.h
        ${I2C_SOURCES}
        ${CMAKE_BINARY_DIR}/geninc/memregmap.cpp)
target_link_libraries(trikSorts1 hetarch hetarch_trik)
target_include_directories(trikSorts1 PUBLIC ${CMAKE_BINARY_DIR}/geninc/)

add_executable(motorsSample motorsSample.cpp
        motors.h
        ${CMAKE_BINARY_DIR}/geninc/memregmap.cpp)
target_link_libraries(motorsSample hetarchcg)
target_include_directories(motorsSample PUBLIC ${CMAKE_BINARY_DIR}/geninc/)

