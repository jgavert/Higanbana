#!/usr/bin/bash

# my own libs
rm -rf ext/higan_coroutines
git clone https://github.com/jgavert/redesigned-giggle.git ext/higan_coroutines

# tests tests tests, not sure if maintained in the long run
rm -rf ext/Catch2
git clone https://github.com/catchorg/Catch2.git ext/Catch2

# commandline options, something for it
rm -rf ext/cxxopts
git clone https://github.com/jarro2783/cxxopts.git ext/cxxopts

# For gsl::span
rm -rf ext/GSL
git clone https://github.com/microsoft/GSL.git ext/GSL

# for easy image loading headers
rm -rf ext/stb
git clone https://github.com/nothings/stb.git ext/stb

# Not going to even explain
rm -rf ext/imgui
git clone https://github.com/ocornut/imgui.git ext/imgui

# switch to docking branch for some sexy docking time
cd ext/imgui
git fetch
git checkout docking
cd ../..

# For all json needs, api looks clean and usable
rm -rf ext/nlohmann_json
git clone https://github.com/nlohmann/json.git ext/nlohmann_json

# gltf loader, could be more efficient if it skipped few copies.
rm -rf ext/cgltf
git clone https://github.com/jkuhlmann/cgltf.git ext/cgltf

# shader compiler for DX12&vulkan dxil&spirv
rm -rf ext/DirectXShaderCompiler
git clone https://github.com/microsoft/DirectXShaderCompiler ext/DirectXShaderCompiler

mkdir -p ext/DirectXShaderCompiler/dxc-bin

# now just build
cd ext/DirectXShaderCompiler
git submodule update --init

# Set up environment
cd dxc-bin
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ $(cat ../utils/cmake-predefined-config-params)
ninja

cd ../../..

rm -rf ext/dxc-bin
mkdir -p ext/dxc-bin/lib
mkdir -p ext/dxc-bin/bin
mkdir -p ext/dxc-bin/include
cp ext/DirectXShaderCompiler/dxc-bin/bin/dxc* ext/dxc-bin/bin/.
cp ext/DirectXShaderCompiler/dxc-bin/lib/libdxcompiler.so* ext/dxc-bin/lib/.
cp -r ext/DirectXShaderCompiler/dxc-bin/include/* ext/dxc-bin/include/.
cp -r ext/DirectXShaderCompiler/include/* ext/dxc-bin/include/.

echo 'You can now remove "ext/DirectXShaderCompiler" if you want to reduce amount of bloat :)'
echo "ext/dxc-bin" has all the relevant data now

echo remember to download Bazel from https://github.com/bazelbuild/bazel/releases
echo "too hard to programmatically get it from there for you :)"

pause