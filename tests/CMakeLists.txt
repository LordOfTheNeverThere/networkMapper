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

include(GoogleTest)
gtest_discover_tests(networkMapperTests)