
# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.6.0)
## cmake_minimum_required(VERSION 3.18.1)

# build application's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++17")

# Declares and names the project.

project("Rev-Lib-WebRTC")

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

## configure import libs
set(REV_LIBS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include_directories(${REV_LIBS_DIR}/rev_json)

add_library(rev_lib_data_channel SHARED IMPORTED)
set_target_properties(rev_lib_data_channel PROPERTIES IMPORTED_LOCATION
        ${REV_LIBS_DIR}/libdatachannel/lib/${ANDROID_ABI}/libdatachannel.so)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

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
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/rev_client
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
