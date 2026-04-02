cmake_minimum_required(VERSION 4.1)
project(networkMapper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(networkMapperLib STATIC
        include/networkMapper/Mapper.h
        src/Mapper.cpp
        src/IPv4Range.cpp
        include/networkMapper/Exceptions.h
        include/networkMapper/IPv4Range.h
        include/networkMapper/TraceRouteResult.h
        include/networkMapper/Tracer.h
        src/Tracer.cpp
        src/TraceRouteResult.cpp
        include/networkMapper/TraceRouteHop.h
        include/networkMapper/MainLogic.h
)
add_subdirectory(../socks ${CMAKE_BINARY_DIR}/socks)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../socks/resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/../socks/resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/tests")
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