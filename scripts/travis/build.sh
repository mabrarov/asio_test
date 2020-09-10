#!/bin/bash

set -e

echo "Preparing build dir at ${BUILD_HOME}"
rm -rf "${BUILD_HOME}"
mkdir -p "${BUILD_HOME}"
cd "${BUILD_HOME}"

cmake_gen_cmd="cmake -D CMAKE_C_COMPILER=\"${C_COMPILER}\" -D CMAKE_CXX_COMPILER=\"${CXX_COMPILER}\" -D CMAKE_BUILD_TYPE=\"${BUILD_TYPE}\""
if [[ -n "${BOOST_HOME+x}" ]]; then
  echo "Building with Boost ${BOOST_VERSION} located at ${BOOST_HOME}"
  cmake_gen_cmd="${cmake_gen_cmd} -D CMAKE_SKIP_BUILD_RPATH=ON -D Boost_NO_SYSTEM_PATHS=ON -D BOOST_INCLUDEDIR=\"${BOOST_HOME}/include\" -D BOOST_LIBRARYDIR=\"${BOOST_HOME}/lib\""
else
  echo "Building with Boost ${BOOST_VERSION} located at system paths"
fi
cmake_gen_cmd="${cmake_gen_cmd} \"${TRAVIS_BUILD_DIR}\""
echo "CMake project generation command: ${cmake_gen_cmd}"
eval "${cmake_gen_cmd}"

build_cmd="cmake --build \"${BUILD_HOME}\" --config \"${BUILD_TYPE}\""
echo "CMake project build command: ${build_cmd}"
eval "${build_cmd}"

test_cmd="\"${BUILD_HOME}/asio_test\" \"${USE_STRAND_WRAP}\" \"${HANDLER_NUM}\" \"${HANDLER_DURATION}\" \"${THREAD_NUM}\""
echo "Running test: ${test_cmd}"
if eval "${test_cmd}" ; then
  echo "Test succeeded"
else
  echo "Test failed"
  exit 1
fi
