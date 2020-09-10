if ((Test-Path env:BOOST_VERSION) -and (${env:BOOST_LINKAGE} -ne "static")) {
  $env:PATH = "${env:BOOST_LIBRARY_FOLDER};${env:PATH}"
}
if (Test-Path env:MINGW_HOME) {
  $env:PATH = "${env:MINGW_HOME}\bin;${env:PATH}"
}
