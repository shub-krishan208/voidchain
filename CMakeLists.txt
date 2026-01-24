cmake_minimum_required(VERSION 3.20)

project(VoidChain VERSION 0.0.1 LANGUAGES CXX)

find_package(Crow CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GTest CONFIG REQUIRED)


add_executable(voidchain src/main.cpp)

target_link_libraries(voidchain PRIVATE
    nlohmann_json::nlohmann_json
    Crow::Crow
    OpenSSL::SSL
    OpenSSL::Crypto
    GTest::gtest_main
)