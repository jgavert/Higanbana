@echo off
cd ..
call .\utils\setEnv.bat
call .\utils\copyDXIL.bat
bazel build test_main --compilation_mode=fastbuild

rd /s /Q releaseDir
mkdir releaseDir

robocopy data releaseDir/data /E
mkdir releaseDir\bazel-bin\test_main
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main test_main.exe
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main dxcompiler.dll
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main WinPixEventRuntime.dll
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main test_main.pdb
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main DXIL.dll
robocopy . releaseDir runDX12.bat
robocopy . releaseDir runDX12_RGP.bat
robocopy . releaseDir runVulkan.bat
robocopy . releaseDir runVulkan_RGP.bat

del higanbana_win64_fastbuild.zip
powershell -Command "Compress-Archive -Path releaseDir\* -DestinationPath higanbana_win64_fastbuild.zip"