@echo off
REM tests tests tests, not sure if maintained in the long run
rd /Q /S ext\Catch2
git clone https://github.com/catchorg/Catch2.git ext/Catch2

REM commandline options, something for it
rd /Q /S ext\cxxopts
git clone https://github.com/jarro2783/cxxopts.git ext/cxxopts

REM For gsl::span
rd /Q /S ext\GSL
git clone https://github.com/microsoft/GSL.git ext/GSL

REM for easy image loading headers
rd /s /Q ext\stb
git clone https://github.com/nothings/stb.git ext/stb

REM Not going to even explain
rd /s /Q ext\imgui
git clone https://github.com/ocornut/imgui.git ext/imgui

REM switch to docking branch for some sexy docking time
cd ext\imgui
git fetch
git checkout docking
cd ..\..

REM For all json needs, api looks clean and usable
rd /s /Q ext\nlohmann_json
git clone https://github.com/nlohmann/json.git ext/nlohmann_json

REM gltf loader, could be more efficient if it skipped few copies.
rd /s /Q ext\tinygltf
git clone https://github.com/syoyo/tinygltf.git ext/tinygltf

REM  https://www.nuget.org/api/v2/package/WinPixEventRuntime
rd /s /Q ext\winpixeventruntime
powershell -Command "Invoke-WebRequest https://www.nuget.org/api/v2/package/WinPixEventRuntime -OutFile ext\wper.zip"
powershell -Command "Expand-Archive ext\wper.zip -DestinationPath ext\winpixeventruntime"
del /Q ext\wper.zip

REM shader compiler for DX12&vulkan dxil&spirv
rd /s /Q ext\DirectXShaderCompiler
git clone https://github.com/microsoft/DirectXShaderCompiler ext/DirectXShaderCompiler

mkdir ext\DirectXShaderCompiler\dxc-bin

REM now just build
cd ext/DirectXShaderCompiler
git submodule update --init

REM Set up environment
call .\utils\hct\hctstart.cmd . dxc-bin	 

REM Run the following if Windows Driver Kit is not installed
call python .\utils\hct\hctgettaef.py

REM # Configure and build
call .\utils\hct\hctbuild.cmd -rel -parallel -spirv

cd ..
cd ..

rmdir /S /Q ext\dxc-bin
mkdir ext\dxc-bin\lib
mkdir ext\dxc-bin\bin
mkdir ext\dxc-bin\include
robocopy ext\DirectXShaderCompiler\dxc-bin\Release\bin ext\dxc-bin\bin dxcompiler.dll
robocopy ext\DirectXShaderCompiler\dxc-bin\Release\lib ext\dxc-bin\lib dxcompiler.lib
robocopy /s ext\DirectXShaderCompiler\dxc-bin\include ext\dxc-bin\include
robocopy /s ext\DirectXShaderCompiler\include ext\dxc-bin\include

echo You can now remove "ext\DirectXShaderCompiler" if you want to reduce amount of bloat :) "ext\dxc-bin" has all the relevant data now

rem remember to download Bazel from https://github.com/bazelbuild/bazel/releases
rem too hard to programmatically get it from there for you :)

pause