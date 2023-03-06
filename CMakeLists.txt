cmake_minimum_required(VERSION 3.6.0)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")

project("Rev-Lib-WebRTC")

find_library(log-lib log)

set(REV_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
include_directories(${REV_LIBS_DIR}/rev_json)
add_library(rev_lib_data_channel SHARED IMPORTED)
set_target_properties(rev_lib_data_channel PROPERTIES IMPORTED_LOCATION
        ${REV_LIBS_DIR}/libdatachannel/lib/${ANDROID_ABI}/libdatachannel.so)

set(REV_CLIENT_SRCS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/rev_client)

set(REV_CLIENT_SRCS
        ${REV_CLIENT_SRCS_SOURCE_DIR}/main.cpp
        ${REV_CLIENT_SRCS_SOURCE_DIR}/rev_client_init.hpp
        ${REV_CLIENT_SRCS_SOURCE_DIR}/rev_client_init.cpp
        ${REV_CLIENT_SRCS_SOURCE_DIR}/parse_cl.cpp
        ${REV_CLIENT_SRCS_SOURCE_DIR}/parse_cl.h
        )

add_library(rev_client SHARED ${REV_CLIENT_SRCS})
target_link_libraries(
        rev_client

        rev_lib_data_channel

        ${log-lib})

target_include_directories(rev_client
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/rev_pers_lib/rev_client
        PUBLIC ${REV_LIBS_DIR}/libdatachannel/include
        PUBLIC ${REV_LIBS_DIR}/rev_json
        )

add_library( # Sets the name of the library.
        Rev-Lib-WebRTC

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/Rev-Lib-WebRTC.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/Rev-Lib-WebRTC.cpp)

target_include_directories(Rev-Lib-WebRTC
        PUBLIC ${REV_LIBS_DIR}/libdatachannel/include
        PUBLIC ${REV_LIBS_DIR}/rev_json
        )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        Rev-Lib-WebRTC

        rev_lib_data_channel

        rev_client

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})
