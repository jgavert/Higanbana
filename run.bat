set mypath=%~dp0
echo %mypath%
pushd %mypath%\data
start %mypath%\bazel-bin\test_main\test_main.exe
popd