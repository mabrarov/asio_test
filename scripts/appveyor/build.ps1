New-Item -Path "${env:BUILD_HOME}" -ItemType "directory" | out-null
Set-Location -Path "${env:BUILD_HOME}"

$cmake_gen_cmd = "cmake"
if (${env:TOOLCHAIN} -eq "mingw") {
  $cmake_gen_cmd = "${cmake_gen_cmd} -D CMAKE_SH=""CMAKE_SH-NOTFOUND"""
}
switch (${env:RUNTIME_LINKAGE}) {
  "static" {
    $cmake_gen_cmd = "${cmake_gen_cmd} -D CMAKE_USER_MAKE_RULES_OVERRIDE=""${env:APPVEYOR_BUILD_FOLDER}\cmake\static_c_runtime_overrides.cmake"" -D CMAKE_USER_MAKE_RULES_OVERRIDE_CXX=""${env:APPVEYOR_BUILD_FOLDER}\cmake\static_cxx_runtime_overrides.cmake"""
  }
}
$cmake_gen_cmd = "${cmake_gen_cmd} -D BOOST_INCLUDEDIR=""${env:BOOST_INCLUDE_FOLDER}"" -D BOOST_LIBRARYDIR=""${env:BOOST_LIBRARY_FOLDER}"" -D Boost_USE_STATIC_LIBS=${env:BOOST_USE_STATIC_LIBS} -D Boost_NO_SYSTEM_PATHS=ON"
if ((${env:TOOLCHAIN} -eq "mingw") -and (Test-Path env:BOOST_PLATFORM_SUFFIX)) {
  $cmake_gen_cmd = "${cmake_gen_cmd} -D Boost_ARCHITECTURE=""${env:BOOST_PLATFORM_SUFFIX}"""
}
if (${env:TOOLCHAIN} -eq "mingw") {
  $cmake_gen_cmd = "${cmake_gen_cmd} -D CMAKE_BUILD_TYPE=""${env:CONFIGURATION}"""
}
$cmake_gen_cmd = "${cmake_gen_cmd} -G ""${env:CMAKE_GENERATOR}"""
if (${env:CMAKE_GENERATOR_PLATFORM}) {
  $cmake_gen_cmd = "${cmake_gen_cmd} -A ""${env:CMAKE_GENERATOR_PLATFORM}"""
}
$cmake_gen_cmd = "${cmake_gen_cmd} ""${env:APPVEYOR_BUILD_FOLDER}"""
Write-Host "CMake project generation command: ${cmake_gen_cmd}"

Invoke-Expression "${cmake_gen_cmd}"
if (${LastExitCode} -ne 0) {
  throw "Generation of project failed with exit code ${LastExitCode}"
}

$build_cmd = "cmake --build . --config ""${env:CONFIGURATION}"""
if ((${env:TOOLCHAIN} -eq "msvc") -and (${env:MSVC_VERSION} -ne "9.0")) {
  $build_cmd = "${build_cmd} -- /maxcpucount /verbosity:normal /logger:""C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"""
}
Write-Host "CMake project build command: ${build_cmd}"

Invoke-Expression "${build_cmd}"
if (${LastExitCode} -ne 0) {
  throw "Build failed with exit code ${LastExitCode}"
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
