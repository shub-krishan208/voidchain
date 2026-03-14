cmake_minimum_required(VERSION 3.20)
project(VoidChain VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UUID REQUIRED IMPORTED_TARGET uuid)

# core library to be reused in apps and subapps (tests)
add_library(voidchain_core 
    src/block.cpp
    src/chain.cpp
    src/router.cpp
    src/p2p_server.cpp
    src/osslWrapper.cpp
    src/wallet.cpp
    src/TxnPool.cpp
    src/miner.cpp
    src/utils/hashing.cpp
    src/core/TxnFactory.cpp
    src/models/CurrencyTxn.cpp
    src/models/AssetTxn.cpp
)

# complete core library dependencies
target_link_libraries(voidchain_core PUBLIC
    nlohmann_json::nlohmann_json
    Crow::Crow
    OpenSSL::SSL
    OpenSSL::Crypto
    PkgConfig::UUID
)

add_executable(voidchain src/main.cpp)
target_link_libraries(voidchain PRIVATE voidchain_core)

option(BUILD_TESTING "Build Tests" ON)

if(BUILD_TESTING)
    # Enable testing
    enable_testing()
    add_subdirectory(tests)
endif()