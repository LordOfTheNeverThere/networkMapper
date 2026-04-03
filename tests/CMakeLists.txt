add_executable(networkMapperTests
        IPv4Range.cpp
        Mapper.cpp
        Tracer.cpp
        MainLogic.cpp
)

target_link_libraries(networkMapperTests
        PRIVATE
        networkMapperLib
        socks
        GTest::gtest_main
)

target_compile_definitions(networkMapperTests PRIVATE
        SOCKS_RESOURCES="${socks_BINARY_DIR}/resources/"
)

include(GoogleTest)
gtest_discover_tests(networkMapperTests)