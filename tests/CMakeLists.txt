# /src/tests/CMakeLists.txt

find_package(GTest CONFIG REQUIRED)
# find_package(CURL CONFIG REQUIRED)

add_executable(run_tests
    block_tests.cpp
    chain_tests.cpp
    miner_tests.cpp
    p2p_tests.cpp
    wallet_tests.cpp
    wallet_api_tests.cpp
    txn_tests.cpp
    txnpool_tests.cpp
    state_tests.cpp
    light_client_tests.cpp
    merkle_tests.cpp
    hashing_test.cpp
    # server_tests.cpp
)

target_link_libraries(run_tests PRIVATE 
    voidchain_core
    # CURL::libcurl
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_tests)