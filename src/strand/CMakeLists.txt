cmake_minimum_required(VERSION 3.0)
project(strand)

find_package(Boost 1.73.0 REQUIRED COMPONENTS
    system
    date_time
    chrono
    regex)

# Boost.Asio uses platform threads internally
find_package(Threads REQUIRED)

set(compile_definitions )
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND cxx_compile_definitions "BOOST_ALL_DYN_LINK")
endif()
if(WIN32)
    list(APPEND compile_definitions
        WIN32
        WIN32_LEAN_AND_MEAN
        WINVER=0x0501
        _WIN32_WINNT=0x0501
        _WIN32_WINDOWS=0x0501
        _WIN32_IE=0x0600
        _UNICODE
        UNICODE
        _WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -municode")
endif()

set(libraries )
list(APPEND libraries ${Boost_LIBRARIES})
list(APPEND libraries ${CMAKE_THREAD_LIBS_INIT})
if(WIN32)
    list(APPEND libraries "ws2_32" "mswsock")
endif()

add_executable(strand src/main.cpp)
target_compile_features(strand PRIVATE cxx_std_11 cxx_lambdas)
target_compile_definitions(strand PRIVATE ${compile_definitions})
target_include_directories(strand PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(strand PRIVATE ${libraries})
