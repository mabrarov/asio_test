cmake_minimum_required(VERSION 3.0)
project(strand_test)

find_package(Boost 1.73.0 REQUIRED COMPONENTS
    system
    date_time
    chrono
    regex
    program_options)

# Boost.Asio uses platform threads internally
find_package(Threads REQUIRED)

set(compile_definitions )
if(NOT Boost_USE_STATIC_LIBS)
    list(APPEND compile_definitions "BOOST_ALL_DYN_LINK")
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
list(APPEND libraries asio_test::latch)
list(APPEND libraries Threads::Threads)
if(WIN32)
    list(APPEND libraries "ws2_32" "mswsock")
endif()

add_executable(strand_test src/main.cpp)
target_compile_features(strand_test PRIVATE cxx_std_11 cxx_lambdas cxx_auto_type)
target_compile_definitions(strand_test PRIVATE ${compile_definitions})
target_include_directories(strand_test PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(strand_test PRIVATE ${libraries})
