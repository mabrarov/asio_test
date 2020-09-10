cmake_minimum_required(VERSION 3.0)
project(latch)

find_package(Boost REQUIRED)

set(compile_definitions )
if(WIN32)
    list(APPEND compile_definitions
        WIN32
        WIN32_LEAN_AND_MEAN
        WINVER=0x0501
        _WIN32_WINNT=0x0501
        _WIN32_WINDOWS=0x0501
        _WIN32_IE=0x0600
        _UNICODE
        UNICODE)
endif()

add_library(latch STATIC include/latch.hpp src/latch.cpp)
add_library(asio_test::latch ALIAS latch)
target_compile_features(latch PUBLIC cxx_std_11)
target_compile_definitions(latch PUBLIC ${compile_definitions})
target_include_directories(latch PUBLIC ${Boost_INCLUDE_DIRS} include)
