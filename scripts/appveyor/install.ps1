$env:OS_VERSION = (Get-WmiObject win32_operatingsystem).version

$vswhere_home = "${env:DEPENDENCIES_FOLDER}\vswhere-${env:VSWHERE_VERSION}"
$vswhere_executable = "${vswhere_home}\${env:VSWHERE_DIST_NAME}"
if (!(Test-Path -Path "${vswhere_executable}")) {
  Write-Host "Visual Studio Locator ${env:VSWHERE_VERSION} not found at ${vswhere_home}"
  $vswhere_archive_file = "${env:DOWNLOADS_FOLDER}\${env:VSWHERE_DIST_NAME}"
  $vswhere_download_url = "${env:VSWHERE_URL}/${env:VSWHERE_VERSION}/${env:VSWHERE_DIST_NAME}"
  if (!(Test-Path -Path "${vswhere_archive_file}")) {
    Write-Host "Downloading Visual Studio Locator ${env:VSWHERE_VERSION} from ${vswhere_download_url} to ${vswhere_archive_file}"
    if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
      New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
    }
    curl.exe `
      --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
      --max-time "${env:CURL_MAX_TIME}" `
      --retry "${env:CURL_RETRY}" `
      --retry-delay "${env:CURL_RETRY_DELAY}" `
      --show-error --silent --location `
      --output "${vswhere_archive_file}" `
      "${vswhere_download_url}"
    if (${LastExitCode} -ne 0) {
      throw "Downloading of Visual Studio Locator failed with exit code ${LastExitCode}"
    }
    Write-Host "Downloading of Visual Studio Locator completed successfully"
  }
  if (!(Test-Path -Path "${vswhere_home}")) {
    New-Item -Path "${vswhere_home}" -ItemType "directory" | out-null
  }
  Write-Host "Copying Visual Studio Locator ${env:VSWHERE_VERSION} from ${vswhere_archive_file} to ${vswhere_home}"
  Copy-Item -Path "${vswhere_archive_file}" -Destination "${vswhere_home}" -Force
  Write-Host "Copying of Visual Studio Locator completed successfully"
}

$toolchain_id = ""
switch (${env:TOOLCHAIN}) {
  "msvc" {
    $env:TOOLCHAIN_ID = "${env:TOOLCHAIN}-${env:MSVC_VERSION}"
    $env:MSVS_HOME = "${env:ProgramFiles(x86)}\Microsoft Visual Studio ${env:MSVC_VERSION}"
    $env:MSVC_VARS_BATCH_FILE = "${env:MSVS_HOME}\VC\vcvarsall.bat"
    $msvs_install_dir = ""
    switch (${env:MSVC_VERSION}) {
      "14.2" {
        $msvs_install_dir = &"${vswhere_executable}" --% -latest -products Microsoft.VisualStudio.Product.Community -version [16.0,17.0) -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
      }
      "14.1" {
        $msvs_install_dir = &"${vswhere_executable}" --% -latest -products Microsoft.VisualStudio.Product.Community -version [15.0,16.0) -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath
      }
    }
    switch (${env:PLATFORM}) {
      "Win32" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars32.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          "14.1" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars32.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          default {
            $env:MSVC_VARS_PLATFORM = "x86"
          }
        }
      }
      "x64" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars64.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          "14.1" {
            $env:MSVC_VARS_BATCH_FILE = "${msvs_install_dir}\VC\Auxiliary\Build\vcvars64.bat"
            $env:MSVC_VARS_PLATFORM = ""
          }
          "14.0" {
            $env:MSVC_VARS_PLATFORM = "amd64"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version: ${env:MSVC_VERSION}"
          }
        }
      }
      default {
        throw "Unsupported platform for ${env:TOOLCHAIN} toolchain: ${env:PLATFORM}"
      }
    }
    $env:ARTIFACT_PATH_SUFFIX = "\${env:CONFIGURATION}\"
  }
  "mingw" {
    $env:TOOLCHAIN_ID = "${env:TOOLCHAIN}-${env:MINGW_VERSION}"
    $mingw_platform_suffix = ""
    $mingw_thread_model_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $mingw_platform_suffix = "i686-"
        $mingw_exception_suffix = "-dwarf"
      }
      "x64" {
        $mingw_platform_suffix = "x86_64-"
        $mingw_exception_suffix = "-seh"
      }
      default {
        throw "Unsupported platform for ${env:TOOLCHAIN} toolchain: ${env:PLATFORM}"
      }
    }
    $env:MINGW_HOME = "C:\mingw-w64\${mingw_platform_suffix}${env:MINGW_VERSION}-posix${mingw_exception_suffix}-rt_v${env:MINGW_RT_VERSION}-rev${env:MINGW_REVISION}\mingw64"
    $env:ARTIFACT_PATH_SUFFIX = "\"
  }
  default {
    throw "Unsupported toolchain: ${env:TOOLCHAIN}"
  }
}
try {
  $detected_cmake_version = (cmake --version 2> $null `
    | Where-Object {$_ -match 'cmake version ([0-9]+\.[0-9]+\.[0-9]+)'}) `
    -replace "cmake version ([0-9]+\.[0-9]+\.[0-9]+)", '$1'
} catch {
  $detected_cmake_version = ""
}
Write-Host "Detected CMake of ${detected_cmake_version} version"
if (Test-Path env:CMAKE_VERSION) {
  Write-Host "CMake of ${env:CMAKE_VERSION} version is requested"
  if ([System.Version] "${env:CMAKE_VERSION}" -ne [System.Version] ${detected_cmake_version}) {
    if ([System.Version] "${env:CMAKE_VERSION}" -ge [System.Version] "3.6.0") {
      $cmake_archive_base_name = "cmake-${env:CMAKE_VERSION}-win64-x64"
    } else {
      # CMake x64 binary is not available for CMake version < 3.6.0
      $cmake_archive_base_name = "cmake-${env:CMAKE_VERSION}-win32-x86"
    }
    $cmake_home = "${env:DEPENDENCIES_FOLDER}\${cmake_archive_base_name}"
    if (!(Test-Path -Path "${cmake_home}")) {
      Write-Host "CMake ${env:CMAKE_VERSION} not found at ${cmake_home}"
      $cmake_archive_name = "${cmake_archive_base_name}.zip"
      $cmake_archive_file = "${env:DOWNLOADS_FOLDER}\${cmake_archive_name}"
      $cmake_download_url = "https://github.com/Kitware/CMake/releases/download/v${env:CMAKE_VERSION}/${cmake_archive_name}"
      if (!(Test-Path -Path "${cmake_archive_file}")) {
        Write-Host "Downloading CMake ${env:CMAKE_VERSION} archive from ${cmake_download_url} to ${cmake_archive_file}"
        if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
          New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
        }
        curl.exe `
          --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
          --max-time "${env:CURL_MAX_TIME}" `
          --retry "${env:CURL_RETRY}" `
          --retry-delay "${env:CURL_RETRY_DELAY}" `
          --show-error --silent --location `
          --output "${cmake_archive_file}" `
          "${cmake_download_url}"
        if (${LastExitCode} -ne 0) {
          throw "Downloading of CMake failed with exit code ${LastExitCode}"
        }
        Write-Host "Downloading of CMake completed successfully"
      }
      if (!(Test-Path -Path "${env:DEPENDENCIES_FOLDER}")) {
        New-Item -Path "${env:DEPENDENCIES_FOLDER}" -ItemType "directory" | out-null
      }
      Write-Host "Extracting CMake ${env:CMAKE_VERSION} from ${cmake_archive_file} to ${env:DEPENDENCIES_FOLDER}"
      7z.exe x "${cmake_archive_file}" -o"${env:DEPENDENCIES_FOLDER}" -aoa -y | out-null
      if (${LastExitCode} -ne 0) {
        throw "Extracting CMake failed with exit code ${LastExitCode}"
      }
      Write-Host "Extracting of CMake completed successfully"
    }
    Write-Host "CMake ${env:CMAKE_VERSION} is located at ${cmake_home}"
    $env:PATH = "${cmake_home}\bin;${env:PATH}"
  }
}
if (Test-Path env:BOOST_VERSION) {
  $pre_installed_boost = $false
  switch (${env:TOOLCHAIN}) {
    "msvc" {
      switch (${env:MSVC_VERSION}) {
        "14.2" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.71.0") `
            -or (${env:BOOST_VERSION} -eq "1.73.0") `
        }
        "14.1" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.69.0") `
            -or (${env:BOOST_VERSION} -eq "1.67.0") `
            -or (${env:BOOST_VERSION} -eq "1.66.0") `
            -or (${env:BOOST_VERSION} -eq "1.65.1") `
            -or (${env:BOOST_VERSION} -eq "1.64.0")
        }
        "14.0" {
          $pre_installed_boost = (${env:BOOST_VERSION} -eq "1.69.0") `
            -or (${env:BOOST_VERSION} -eq "1.67.0") `
            -or (${env:BOOST_VERSION} -eq "1.66.0") `
            -or (${env:BOOST_VERSION} -eq "1.65.1") `
            -or (${env:BOOST_VERSION} -eq "1.63.0") `
            -or (${env:BOOST_VERSION} -eq "1.62.0") `
            -or (${env:BOOST_VERSION} -eq "1.60.0")
        }
      }
    }
  }
  if (${pre_installed_boost}) {
    $boost_home = "C:\Libraries\boost"
    if (!(${env:BOOST_VERSION} -eq "1.56.0")) {
      $boost_home_version_suffix = "_${env:BOOST_VERSION}" -replace "\.", '_'
      $boost_home = "${boost_home}${boost_home_version_suffix}"
    }
    $boost_folder_platform_suffix = ""
    switch (${env:PLATFORM}) {
      "Win32" {
        $boost_folder_platform_suffix = "lib32"
      }
      "x64" {
        $boost_folder_platform_suffix = "lib64"
      }
      default {
        throw "Unsupported platform for Boost: ${env:PLATFORM}"
      }
    }
    $boost_folder_toolchain_suffix = "-msvc-${env:MSVC_VERSION}"
    $env:BOOST_INCLUDE_FOLDER = "${boost_home}"
    $env:BOOST_LIBRARY_FOLDER = "${boost_home}\${boost_folder_platform_suffix}${boost_folder_toolchain_suffix}"
  } else {
    switch (${env:PLATFORM}) {
      "Win32" {
        $env:BOOST_PLATFORM_SUFFIX = "-x86"
      }
      "x64" {
        $env:BOOST_PLATFORM_SUFFIX = "-x64"
      }
      default {
        throw "Unsupported platform for Boost: ${env:PLATFORM}"
      }
    }
    $boost_version_suffix = "-${env:BOOST_VERSION}"
    $boost_toolchain_suffix = ""
    switch (${env:TOOLCHAIN}) {
      "msvc" {
        switch (${env:MSVC_VERSION}) {
          "14.2" {
            $boost_toolchain_suffix = "-vs2019"
          }
          "14.1" {
            $boost_toolchain_suffix = "-vs2017"
          }
          "14.0" {
            $boost_toolchain_suffix = "-vs2015"
          }
          default {
            throw "Unsupported ${env:TOOLCHAIN} version for Boost: ${env:MSVC_VERSION}"
          }
        }
      }
      "mingw" {
        $boost_toolchain_suffix = "-mingw${env:MINGW_VERSION}" -replace "([\d]+)\.([\d]+)\.([\d]+)", '$1$2'
      }
      default {
        throw "Unsupported toolchain for Boost: ${env:TOOLCHAIN}"
      }
    }
    $boost_install_folder = "${env:DEPENDENCIES_FOLDER}\boost${boost_version_suffix}${env:BOOST_PLATFORM_SUFFIX}${boost_toolchain_suffix}"
    if (!(Test-Path -Path "${boost_install_folder}")) {
      Write-Host "Boost is absent for the chosen toolchain (${env:TOOLCHAIN_ID}) and Boost version (${env:BOOST_VERSION}) at ${boost_install_folder}"
      $boost_archive_name = "boost${boost_version_suffix}${env:BOOST_PLATFORM_SUFFIX}${boost_toolchain_suffix}.7z"
      $boost_archive_file = "${env:DOWNLOADS_FOLDER}\${boost_archive_name}"
      if (!(Test-Path -Path "${boost_archive_file}")) {
        $boost_download_url = "https://dl.bintray.com/mabrarov/generic/boost/${env:BOOST_VERSION}/${boost_archive_name}"
        if (!(Test-Path -Path "${env:DOWNLOADS_FOLDER}")) {
          New-Item -Path "${env:DOWNLOADS_FOLDER}" -ItemType "directory" | out-null
        }
        Write-Host "Downloading Boost from ${boost_download_url} to ${boost_archive_file}"
        curl.exe `
          --connect-timeout "${env:CURL_CONNECT_TIMEOUT}" `
          --max-time "${env:CURL_MAX_TIME}" `
          --retry "${env:CURL_RETRY}" `
          --retry-delay "${env:CURL_RETRY_DELAY}" `
          --show-error --silent --location `
          --output "${boost_archive_file}" `
          "${boost_download_url}"
        if (${LastExitCode} -ne 0) {
          throw "Downloading of Boost failed with exit code ${LastExitCode}"
        }
        Write-Host "Downloading of Boost completed successfully"
      }
      Write-Host "Extracting Boost from ${boost_archive_file} to ${env:DEPENDENCIES_FOLDER}"
      if (!(Test-Path -Path "${env:DEPENDENCIES_FOLDER}")) {
        New-Item -Path "${env:DEPENDENCIES_FOLDER}" -ItemType "directory" | out-null
      }
      7z.exe x "${boost_archive_file}" -o"${env:DEPENDENCIES_FOLDER}" -aoa -y | out-null
      if (${LastExitCode} -ne 0) {
        throw "Extracting of Boost failed with exit code ${LastExitCode}"
      }
      Write-Host "Extracting of Boost completed successfully"
    }
    Write-Host "Boost ${env:BOOST_VERSION} is located at ${boost_install_folder}"
    $boost_include_folder_version_suffix = "-${env:BOOST_VERSION}" -replace "([\d]+)\.([\d]+)(\.[\d]+)*", '$1_$2'
    $env:BOOST_INCLUDE_FOLDER = "${boost_install_folder}\include\boost${boost_include_folder_version_suffix}"
    $env:BOOST_LIBRARY_FOLDER = "${boost_install_folder}\lib"
  }
  if ((${env:RUNTIME_LINKAGE} -eq "static") -and (${env:BOOST_LINKAGE} -ne "static")) {
    throw "Incompatible type of linkage of Boost: ${env:BOOST_LINKAGE} for the specified type of linkage of C/C++ runtime: ${env:RUNTIME_LINKAGE}"
  }
  switch (${env:BOOST_LINKAGE}) {
    "static" {
      $env:BOOST_USE_STATIC_LIBS = "ON"
    }
    default {
      $env:BOOST_USE_STATIC_LIBS = "OFF"
    }
  }
}
switch (${env:CONFIGURATION}) {
  "Debug" {
    $env:CMAKE_BUILD_CONFIG = "DEBUG"
  }
  "Release" {
    $env:CMAKE_BUILD_CONFIG = "RELEASE"
  }
  default {
    throw "Unsupported build configuration: ${env:CONFIGURATION}"
  }
}
$env:CMAKE_GENERATOR_PLATFORM = ""
switch (${env:TOOLCHAIN}) {
  "msvc" {
    $cmake_generator_msvc_version_suffix = " ${env:MSVC_VERSION}" -replace "([\d]+)\.([\d]+)", '$1'
    switch (${env:MSVC_VERSION}) {
      "14.2" {
        $cmake_generator_msvc_version_suffix = " 16 2019"
      }
      "14.1" {
        $cmake_generator_msvc_version_suffix = " 15 2017"
      }
      "14.0" {
        $cmake_generator_msvc_version_suffix = "${cmake_generator_msvc_version_suffix} 2015"
      }
      default {
        throw "Unsupported ${env:TOOLCHAIN} version for CMake generator: ${env:MSVC_VERSION}"
      }
    }
    switch (${env:PLATFORM}) {
      "Win32" {
        $env:CMAKE_GENERATOR_PLATFORM = "Win32"
      }
      "x64" {
        $env:CMAKE_GENERATOR_PLATFORM = "x64"
      }
      default {
        throw "Unsupported platform for CMake generator: ${env:PLATFORM}"
      }
    }
    $env:CMAKE_GENERATOR = "Visual Studio${cmake_generator_msvc_version_suffix}"
  }
  "mingw" {
    $env:CMAKE_GENERATOR = "MinGW Makefiles"
  }
  "default" {
    throw "Unsupported toolchain for CMake generator: ${env:TOOLCHAIN}"
  }
}
Write-Host "PLATFORM                  : ${env:PLATFORM}"
Write-Host "CONFIGURATION             : ${env:CONFIGURATION}"
Write-Host "TOOLCHAIN_ID              : ${env:TOOLCHAIN_ID}"
switch (${env:TOOLCHAIN}) {
  "msvc" {
    Write-Host "MSVC_VARS_BATCH_FILE      : ${env:MSVC_VARS_BATCH_FILE}"
    Write-Host "MSVC_VARS_PLATFORM        : ${env:MSVC_VARS_PLATFORM}"
  }
  "mingw" {
    Write-Host "MINGW_HOME                : ${env:MINGW_HOME}"
  }
}
Write-Host "APPVEYOR_BUILD_FOLDER     : ${env:APPVEYOR_BUILD_FOLDER}"
Write-Host "ARTIFACT_PATH_SUFFIX      : ${env:ARTIFACT_PATH_SUFFIX}"
Write-Host "BOOST_INCLUDE_FOLDER      : ${env:BOOST_INCLUDE_FOLDER}"
Write-Host "BOOST_LIBRARY_FOLDER      : ${env:BOOST_LIBRARY_FOLDER}"
Write-Host "BOOST_USE_STATIC_LIBS     : ${env:BOOST_USE_STATIC_LIBS}"
Write-Host "CMAKE_GENERATOR           : ${env:CMAKE_GENERATOR}"
Write-Host "CMAKE_GENERATOR_PLATFORM  : ${env:CMAKE_GENERATOR_PLATFORM}"

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
