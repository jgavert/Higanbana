# ???
Aim is to provide simplified, but efficient Graphics API to use the latest and greatest features with ease.

## Building
You will need the following additional software to build.
* Bazel
* Visual Studio 2019
* Vulkan SDK

These are required for building DirectXShaderCompiler
* CMake
* Python 3

### Build Steps - Windows
First run 

    fetchExternalsAndBuildDXC.bat

This will ensure that you have all the externals.

small note: This compiles DXC in release mode with spirv, it takes long. like 10minutes or worse.

Building the project is simply

    bazel build test_main

Which results in my test exe. You can test it by running:

    run.bat
or go to bazel-bin/test_main and run the exe from that location.


## Features
These are partly current features and features I'm aiming for and might have gotten one single case to work. Like with Interop between DX12 and Vulkan.
* Designed for ease of use
    * Triangle doesn't need 1500 lines, maybe.. 50? 30? including shader code TODO: update with correct linecount
* DX12 and Vulkan in one exe through single api
* Multi GPU support through explicit devices
    * Won't take advantage of SLI/Crossfire support
    * This is on-going effort
    * Cross-device isn't well supported by Vulkan on Windows
    * DX12 <-> Vulkan is within one device only
* Easy to make use of Graphics/Compute/Dma queue
    * all semaphores/synchronization/everything handled for you.
* Homebrew implementations to many problems :D

## What's next?
* Write lots of tests
* Clean unnecessary comments and code
* Support Raytracing
* Sharing buffer/image between DX12 and Vulkan for debuggability
    * Use one code to render on both and show the results in one swapchain buffer
* Mesh Shaders
    * Waiting for support from DX12 and DXC
* Optimise the cpu code
    * Multithreading and speed was though when designing the architechture
    * Many low hanging fruits
* Write benchmarks for cpu and maybe for gpu also
    * So that I can have good reference for when I optimise    
* Make many current features as optional
    * Opt-out threading with possibility to integrate with external system
    * Opt-out filesystem support with possibility to integrate with external system
        * Should be possible to only operate in-memory.
        * Mostly shader related.
    * Some kind of error callback like vulkan has for error reporting