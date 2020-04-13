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

pause