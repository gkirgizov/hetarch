
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

add_executable(localLoader localLoader.cpp
        ${TCP_SOURCES})
target_link_libraries(localLoader ${CMAKE_THREAD_LIBS_INIT})


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

