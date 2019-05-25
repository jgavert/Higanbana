REM set mypath=%~dp0
REM echo %mypath%
REM pushd %mypath%\data
start %mypath%\bazel-bin\test_main\test_main.exe
REM popd