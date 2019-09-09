@echo off
cd ..
call .\utils\setEnv.bat
bazel build test_main --compilation_mode=fastbuild

rd /s /Q releaseDir
mkdir releaseDir

robocopy data releaseDir/data /E
mkdir releaseDir\bazel-bin\test_main
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main test_main.exe
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main dxcompiler.dll
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main WinPixEventRuntime.dll
robocopy bazel-bin\test_main releaseDir\bazel-bin\test_main test_main.pdb
robocopy "C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64" "releaseDir\bazel-bin\test_main" DXIL.dll
robocopy . releaseDir run.bat
robocopy . releaseDir runWithRGPDX12.bat
robocopy . releaseDir runWithRGPVulkan.bat

del higanbana_win64_fastbuild.zip
powershell -Command "Compress-Archive -Path releaseDir -DestinationPath higanbana_win64_fastbuild.zip"