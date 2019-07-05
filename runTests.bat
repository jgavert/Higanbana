@echo off
call .\utils\setEnv.bat
bazel test tests:all-tests
pause