@echo off
call .\utils\setEnv.bat
bazel build test_main
pause