cmake_minimum_required(VERSION 3.20)
project(VoidChain VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# core library to be reused in apps and subapps (tests)
add_library(voidchain_core 
    src/block.cpp
    src/chain.cpp
    src/router.cpp
    src/p2p_server.cpp
    src/utils/hashing.cpp
)

# complete core library dependencies
target_link_libraries(voidchain_core PUBLIC
    nlohmann_json::nlohmann_json
    Crow::Crow
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_executable(voidchain src/main.cpp)
target_link_libraries(voidchain PRIVATE voidchain_core)

option(BUILD_TESTING "Build Tests" ON)

if(BUILD_TESTING)
    # Enable testing
    enable_testing()
    add_subdirectory(tests)
endif()