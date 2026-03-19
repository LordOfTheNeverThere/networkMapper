cmake_minimum_required(VERSION 4.1)
project(networkMapper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(networkMapperLib STATIC
        include/networkMapper/Mapper.h
        src/Mapper.cpp
        src/IPv4Range.cpp
)
add_subdirectory(../socks ${CMAKE_BINARY_DIR}/socks)
target_link_libraries(networkMapperLib PRIVATE socks)


add_executable(networkMapperDev src/main.cpp
        src/main.cpp)
target_link_libraries(networkMapperDev PRIVATE networkMapperLib)


target_include_directories(networkMapperLib
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # Allows /src/*.cpp files to find local headers easily
)

# GTEST

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(tests)