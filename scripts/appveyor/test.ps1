$test_cmd = "& ""${env:BUILD_HOME}\src\strand_test${env:ARTIFACT_PATH_SUFFIX}strand_test.exe"" --use-strand-wrap ""${env:STRAND_TEST_USE_STRAND_WRAP}"" --threads ""${env:STRAND_TEST_THREAD_NUM}"" --duration ""${env:STRAND_TEST_HANDLER_DURATION}"" --init ""${env:STRAND_TEST_INIT_HANDLER_NUM}"" --concurrent ""${env:STRAND_TEST_CONCURRENT_HANDLER_NUM}"" --strand ""${env:STRAND_TEST_STRAND_HANDLER_NUM}"""
Write-Host "Running test: ${test_cmd}"
Invoke-Expression "${test_cmd}"
if (${LastExitCode} -eq 0) {
  Write-Host "Test succeeded"
} else {
  throw "Test failed"
}

Set-Location -Path "${env:APPVEYOR_BUILD_FOLDER}"
