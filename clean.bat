@echo off
call .\utils\setEnv.bat
bazel clean --expunge
pause