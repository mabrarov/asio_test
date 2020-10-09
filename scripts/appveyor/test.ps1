$test_cmd = "& ""${env:BUILD_HOME}\src\strand_test${env:ARTIFACT_PATH_SUFFIX}strand_test.exe"" --threads ""${env:STRAND_TEST_THREAD_NUM}"" --streams ""${env:STRAND_TEST_STREAM_NUM}"" --size ""${env:STRAND_TEST_OPERATION_SIZE}"" --operations ""${env:STRAND_TEST_OPERATION_NUM}"" --repeats ""${env:STRAND_TEST_REPEAT_NUM}"""
Write-Host "Running test: ${test_cmd}"
Invoke-Expression "${test_cmd}"
if (${LastExitCode} -eq 0) {
  Write-Host "Test succeeded"
} else {
  throw "Test failed"
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
