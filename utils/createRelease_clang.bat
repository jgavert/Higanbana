@echo off
cd ..
call .\utils\setEnv.bat
set USE_CLANG_CL=1
bazel build test_main --compilation_mode=opt

set location="releaseDir_clang"
set outputZip="higanbana_win64_fastbuild_clang"

rd /s /Q %location%
mkdir %location%

robocopy data %location%/data /E
mkdir %location%\bazel-bin\test_main
robocopy bazel-bin\test_main %location%\bazel-bin\test_main test_main.exe
robocopy bazel-bin\test_main %location%\bazel-bin\test_main dxcompiler.dll
robocopy bazel-bin\test_main %location%\bazel-bin\test_main WinPixEventRuntime.dll
robocopy bazel-bin\test_main %location%\bazel-bin\test_main test_main.pdb
robocopy bazel-bin\test_main %location%\bazel-bin\test_main DXIL.dll
robocopy . %location% runDX12.bat
robocopy . %location% runDX12_RGP.bat
robocopy . %location% runVulkan.bat
robocopy . %location% runVulkan_RGP.bat

del %outputZip%.zip
powershell -Command "Compress-Archive -Path %location%\* -DestinationPath %outputZip%.zip"