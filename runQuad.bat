@echo off
cd bazel-bin\test_main
start test_main.exe "1" "1"
start test_main.exe "2" "2"
start test_main.exe "2" "1"
start test_main.exe "1" "2"