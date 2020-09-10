$test_cmd = "& ""${env:BUILD_HOME}${env:ARTIFACT_PATH_SUFFIX}asio_test.exe"" ""${env:USE_STRAND_WRAP}"" ""${env:HANDLER_NUM}"" ""${env:THREAD_NUM}"""
Write-Host "Running test: ${test_cmd}"
Invoke-Expression "${test_cmd}"
if (${LastExitCode} -eq 0) {
  Write-Host "Test succeeded"
} else {
  throw "Test failed"
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
