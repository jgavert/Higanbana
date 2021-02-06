@echo off
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
call .\utils\hct\hctbuild.cmd -rel -vs2019 -spirv

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