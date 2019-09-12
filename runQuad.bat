@echo off
cd bazel-bin\test_main
start test_main.exe --dx12 --amd
start test_main.exe --vulkan --amd
start test_main.exe --dx12 --nvidia
start test_main.exe --vulkan --nvidia