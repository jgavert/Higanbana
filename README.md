# Higanbana
as the flower depicts, is a library which grows in the lands of Vulkan and DirectX12. Lands which are not very kind to mortals...

Aim is to provide simplified, but efficient Graphics API, helping with using the latest and greatest gpu features...

Higanbana is mostly api for my personal use, ease researching and testing new rendering techniques, also proof of concept on understanding the api's.

## Building
You will need the following additional software to build.
* Windows 10 build 1809 or higher
* Bazel
* Visual Studio 2019
* Windows SDK 10.0.18368.0
* Latest Vulkan SDK
* Latest GPU Drivers

Optional
* VS Code with c++ plugins...

These are required for building DirectXShaderCompiler
* CMake
* Python 3

### Build Steps - Windows
First run 

    fetchExternalsAndBuildDXC.bat

This will ensure that you have all the externals.

small note: This compiles DXC in release mode with spirv. This can take fairly long and prevent you from doing anything on computer for 10 minutes or so.

Building the project is simply

    bazel build test_main
or run build.bat

Which results in my test exe. You can test it by running:

    run.bat
or go to bazel-bin/test_main and run the exe from that location.


## Features
These are partly current features and features I'm aiming for and might have gotten one single case to work. Like with Interop between DX12 and Vulkan.
* Designed for ease of use
    * TODO: write a test to draw triangle
* DX12 and Vulkan in one exe through single api
* Multi GPU support through explicit devices
    * Won't take advantage of SLI/Crossfire support
    * This is on-going effort
    * Cross-device isn't well supported by Vulkan on Windows
    * DX12 <-> Vulkan is within one device only
* Easy to make use of graphics/Compute/Dma queue
    * all semaphores/synchronization/everything handled for you.
* Homebrew implementations to many problems :D
* CPU and GPU timings can be fetched for easy profiling or integrate it to other platforms like chrome://tracing

## What's next?
* Write a forward(+) renderer using my api
    * To get out of this backend writing...
    * Proof that the api can handle the problem.
* Write lots of tests
* Clean unnecessary comments and code
* Support Raytracing
* Sharing buffer/image between DX12 and Vulkan for debuggability
    * Use one code to render on both and show the results in one swapchain buffer
* Mesh Shaders
    * Waiting for support from DX12 and DXC
* Optimise the cpu code
    * Architecture was designed with multithreading in mind, just didn't bother to write it yet.
    * Many low hanging fruits
    * Optimising as I see necessary
* Write benchmarks for cpu and maybe for gpu also
    * So that I can have good reference for when I optimise    
* Make many current features as optional
    * Opt-out threading with possibility to integrate with external system
    * Opt-out filesystem support with possibility to integrate with external system
        * Should be possible to only operate in-memory.
        * Mostly shader related.
    * Some kind of error callback like vulkan has for error reporting
* Linux support
    * This is also the reason why I'm using Bazel as build system
    * Biggest show stoppers here are only...
        * I'm lazy
        * Windowing support abstraction and support
            * honestly bonus, I should start with headless
        * Does DXC compile nicely? make Bazel scripts for it?
        * Really not many problems though, someday!