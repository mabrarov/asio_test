# Tests for Boost.Asio

[![Travis CI build status](https://travis-ci.org/mabrarov/asio_test.svg?branch=master)](https://travis-ci.org/mabrarov/asio_test) [![AppVeyor CI build status](https://ci.appveyor.com/api/projects/status/7avx0sy3ec3d4eq5/branch/master?svg=true)](https://ci.appveyor.com/project/mabrarov/asio-test)  

## Building

### Requirements

1. C++ toolchain
   * Microsoft Visual Studio 2015-2019
   * MinGW 7+
   * GCC 6+
   * Clang 6+
1. [CMake](https://cmake.org/) 3.2+
1. Boost C++ Libraries 1.73.0+

### Assumptions

1. `asio_test_home` environment variable contains path to local copy of
   this repository
1. `build_dir` environment variable specifies build directory, 
   it may be different than `asio_test_home` for "out of source tree" build, 
   it may be `build` subdirectory of `asio_test_home`
1. If Boost C++ Libraries are taken not from system paths, then header files of
   Boost are located at directory specified by `boost_headers_dir` environment
   variable and binary files of Boost are located at directory specified by
   `boost_libs_dir` environment variable
1. `build_type` environment variable specifies [CMake build type](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)
   * `Debug`
   * `Release`
   * `RelWithDebInfo`
   * `MinSizeRel`
1. `cmake_generator` environment variable is [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html)
   * `Visual Studio 14 2015` - Visual Studio 2015
   * `Visual Studio 15 2017` - Visual Studio 2017
   * `Visual Studio 16 2019` - Visual Studio 2019
   * `NMake Makefiles` - NMake makefiles
   * `MinGW Makefiles` - MinGW makefiles
   * `Unix Makefiles` - Unix / Linux makefiles
1. `cmake_platform` environment variable is [CMAKE_GENERATOR_PLATFORM](https://cmake.org/cmake/help/latest/variable/CMAKE_GENERATOR_PLATFORM.html)
   * `Win32` - x86 platform when using Visual Studio
   * `x64` - amd64 (x64) platform when using Visual Studio
1. Unix / Linux commands use Bash
1. Windows commands use Windows Command Prompt

## Steps

1. Generate project for build system from CMake project,
   assuming current directory is `build_dir`

   * Unix / Linux, Boost C++ Libraries are taken from system paths

     ```bash
     cmake_generator="Unix Makefiles" && \
     cmake -D CMAKE_BUILD_TYPE="${build_type}" -G "${cmake_generator}" "${asio_test_home}"
     ```

   * Windows, Visual Studio 2015 x64 CMake generator, shared C/C++ runtime,
     Boost C++ Libraries are taken not from system paths and are linked
     statically (refer to
     [FindBoost](http://www.cmake.org/cmake/help/latest/module/FindBoost.html?highlight=findboost)
     CMake module for CMake variables which can be used to specify the way
     search for Boost C++ Libraries is performed)

     ```cmd
     set "cmake_generator=Visual Studio 14 2015"
     set "cmake_platform=x64"
     cmake ^
     -D Boost_NO_SYSTEM_PATHS=ON ^
     -D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
     -D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
     -D Boost_USE_STATIC_LIBS=ON ^
     -G "%cmake_generator%" -A "%cmake_platform%" "%asio_test_home%"
     ```

   * Windows, Visual Studio 2015 x64 CMake generator, static C/C++ runtime,
     Boost C++ Libraries are taken not from system paths and are linked
     statically

     ```cmd
     set "cmake_generator=Visual Studio 14 2015"
     set "cmake_platform=x64"
     cmake ^
     -D CMAKE_USER_MAKE_RULES_OVERRIDE="%asio_test_home%\cmake\static_c_runtime_overrides.cmake" ^
     -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%asio_test_home%\cmake\static_cxx_runtime_overrides.cmake" ^
     -D Boost_NO_SYSTEM_PATHS=ON ^
     -D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
     -D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
     -D Boost_USE_STATIC_LIBS=ON ^
     -G "%cmake_generator%" -A "%cmake_platform%" "%asio_test_home%"
     ```

   * Windows, MinGW makefiles CMake generator, static C/C++ runtime,
     Boost C++ Libraries are taken not from system paths and are linked
     statically

     ```cmd
     set "cmake_generator=MinGW Makefiles"
     cmake ^
     -D CMAKE_USER_MAKE_RULES_OVERRIDE="%asio_test_home%\cmake\static_c_runtime_overrides.cmake" ^
     -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX="%asio_test_home%\cmake\static_cxx_runtime_overrides.cmake" ^
     -D Boost_NO_SYSTEM_PATHS=ON ^
     -D BOOST_INCLUDEDIR="%boost_headers_dir%" ^
     -D BOOST_LIBRARYDIR="%boost_libs_dir%" ^
     -D Boost_USE_STATIC_LIBS=ON ^
     -D CMAKE_BUILD_TYPE="%build_type%" ^
     -G "%cmake_generator%" "%asio_test_home%"
     ```

1. Build generated project with build system chosen at previous step via CMake generator

   * Unix / Linux

     ```bash
     cmake --build "${build_dir}" --config "${build_type}"
     ```

   * Windows

     ```cmd
     cmake --build "%build_dir%" --config "%build_type%"
     ```

## Usage

Refer to

1. [src/strand/README.md](src/strand/README.md)
