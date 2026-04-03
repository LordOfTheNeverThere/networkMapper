cmake_minimum_required(VERSION 3.28)
project(networkMapper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- FETCH DEPENDENCIES ---

include(FetchContent)
# Fetch socks library
FetchContent_Declare(
        socks
        GIT_REPOSITORY https://github.com/LordOfTheNeverThere/socks.git
        GIT_TAG        master
)
FetchContent_MakeAvailable(socks)

# Fetch GoogleTest
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

# --- TARGETS ---

add_library(networkMapperLib STATIC
        include/networkMapper/Mapper.h
        src/Mapper.cpp
        src/IPv4Range.cpp
        include/networkMapper/IPv4Range.h
        include/networkMapper/TraceRouteResult.h
        include/networkMapper/Tracer.h
        src/Tracer.cpp
        src/TraceRouteResult.cpp
        include/networkMapper/TraceRouteHop.h
        include/networkMapper/MainLogic.h
)

# Link against the fetched socks library
target_link_libraries(networkMapperLib PRIVATE socks)

target_include_directories(networkMapperLib
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${socks_BINARY_DIR}/generated>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src  # Allows /src/*.cpp files to find local headers easily
)

# Main Application Executable
add_executable(networkMapperDev src/main.cpp)
target_link_libraries(networkMapperDev PRIVATE networkMapperLib)

# --- INSTALLATION LOGIC ---

# Install the app binary
install(TARGETS networkMapperDev RUNTIME DESTINATION bin)

# Install the static library (optional, only if you plan on using networkMapper as Library)
#install(TARGETS networkMapperLib ARCHIVE DESTINATION lib)

# Install your headers (optional, only if you plan on using networkMapper as Library)
#install(DIRECTORY include/networkMapper DESTINATION include)

# GTEST
enable_testing()
add_subdirectory(tests)