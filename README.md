# Higanbana (name subject to change)
Aim is to provide simplified, but efficient Graphics API, helping with using the latest and greatest gpu features...

Higanbana is mostly api for my personal use, ease researching and testing new rendering techniques, also my proof of concept on understanding the api's...

## Building
You will need the following additional software to build.
* Windows 10 build 1909 or higher
* Bazel
* Visual Studio 2019
  * Desktop Development with C++
* Windows SDK 10.0.19041.0 or newer
* Latest Vulkan SDK(1.2.162.1)
* Latest GPU Drivers

Optional
* VS Code with c++ plugins...

These are required for building DirectXShaderCompiler
* Visual Studio 2019
  * Universal Windows Platform Development
* CMake
* Python 3
* Windows Driver Kit - same version as the SDK

### Build Steps - Windows
First run 

    fetchExternalsAndBuildDXC.bat

This will ensure that you have all the externals.

small note: This compiles DXC in release mode with spirv. This can take fairly long and prevent you from doing anything on computer for 10 minutes or so. And DXC compilation is highly likely to fail currently. Blame DXC for being so hard to compile.

Building the project is simply

    bazel build test_main
or run build.bat

Which results in my test exe. You can test it by running:

    runDX12.bat or runVulkan.bat
or go to bazel-bin/test_main and run the exe from that location.

## Features
These are partly current features and features I'm aiming for and might have gotten one single case to work. Like with Interop between DX12 and Vulkan.
Slighly out-of-date
* Designed for ease of use
    * TODO: write a test to draw triangle
* DX12 and Vulkan in one exe through single api
* Multi GPU support through explicit devices
    * Won't take advantage of SLI/Crossfire support
    * This is on-going effort
    * Cross-device isn't supported on Vulkan
    * DX12 <-> Vulkan is within one device only, Nvidia only for now.
* Easy to make use of graphics/Compute/Dma queue
    * all semaphores/synchronization/everything handled for you.
* Multithreaded using std::for_each(std::execution:par_unseq !
* Tight cpu perf with simple drawcalls!
    * Warning: Using many resources in shaders is untested perfwise.
* Homebrew implementations to many problems :D
* CPU and GPU timings can be fetched for easy profiling or integrate it to other platforms like chrome://tracing
  * requires compiletime switch currently and default disabled.

## What's next?
* Update Features&What's next? lists here on their current state...
* Profiling support(HighPrio) using chrome://tracing
    * Good profiling is the backbone of any app also I need it for optimizing multithreading timings.
* Write a forward(+) renderer using my api
    * To get out of this backend writing...
    * Proof that the api can handle the problem.
* Write lots of tests
* Clean unnecessary comments and code
* Support Raytracing (Started)
* Sharing buffer/image between DX12 and Vulkan for debuggability
    * Use one code to render on both and show the results in one swapchain buffer (Done)
    * Works only on Nvidia now.
* Mesh Shaders (Mostly done)
    * DX12: Waiting for Windows to update to latest.
* Optimise the cpu code
    * Architecture was designed with multithreading in mind, just didn't bother to write it yet. (DONE)
    * Many low hanging fruits (Most done)
    * Optimising as I see necessary (Some parts have unknown impact, yet to optimize)
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
        * Windowing support abstraction and support
            * honestly bonus, I should start with headless
        * DXC compiles on linux, but have issues linking to it.
        * Effort was started. (Done)

## External libraries
* https://github.com/catchorg/Catch2.git for tests
* https://github.com/jarro2783/cxxopts.git for commandline options
* https://github.com/microsoft/GSL.git for gsl::span
* https://github.com/nothings/stb.git for easy image loading
* https://github.com/ocornut/imgui.git the best and easiest ui library
* https://github.com/nlohmann/json.git for json support
* https://github.com/syoyo/tinygltf.git for gltf parsing
* https://www.nuget.org/api/v2/package/WinPixEventRuntime to support pix events
* https://github.com/microsoft/DirectXShaderCompiler for compiling shaders for Vulkan and DirectX 12
* SpookyV2 hash library
* https://github.com/martinus/robin-hood-hashing/ hashmap
