# Faze
Name depicts that the projects goal isn't clear, hence its referencing mist. (There apparently already is a Haze engine so I'll just use 'F' then :) )
It's a collection of libraries to fasten researching or prototyping of multithreaded and/or advanced graphics applications using atleast D3D12(Main target is vulkan, but yet to appear).

###Requirements:
- Windows 10 (until vulkan comes...?)
- Visual Studio 2015 Community and Windows SDK 10
- Fastbuild binary copied to root folder.
- Reasonable graphics card if using d3d12.
- Oculus SDK(does nothing yet, look into externals/externals.bff for hints what the build system expect)

## Origins for some of the algorithms and motivation:
- Lazy binary-splitting: a run-time adaptive work-stealing scheduler http://www.umiacs.umd.edu/publications/lazy-binary-splitting-run-time-adaptive-work-stealing-scheduler
- The Scheduler is currently deadlock free, only deadlocks have been seen when being attached in visual studio debugger... and the deadlock solves by pausing and continuing...
- Data Oriented Design and Entity systems
- Gpu Driven Rendering. Ps4 Dreams(pointclouds). Various Demoscene productions, mostly those with lots of particles.
- Distancefields and triangilize them on fly with gpu -> Histopyramid

### "Nice to have" features, haven't had time or interest.
- Audio
- Extend window and input support for windows.


### In development
Writing graphics api wrapper for dx12 which hopefully matches vulkan also. Main motivation is to hide most of the manual tasks behind oneliners and bring everything to highlevel without sacrificing control or performance.
Also writing the api using TDD. How I want to use the api depicts the design.

## Needs refactorings
Tasksystem needs a proper nodegraph. Helps debugging and makes dependency handling easier. Performance is good when the unique task name count is low.

### Not in active development, but updated when they break or don't fulfill perfectly the task.
- Math library, Basic but fullfills the needed tasks. Has some tests.
- InputBuffer, Waiting for windower implementation so that I can look at how to even get inputs on windows.
- EntitySystem, Pretty good. Has support for the tasksystem to do parallellized queries into it. Doesn't really bring performance benefits unless the workload is "significant". Should always help performance if tasksystem is used extensively.
- Internal messaging system which supports sending messages to any subsystem desired from anywhere. Shouldn't be used unless the usecase is perfect for it.
- "Print" macro that uses the messaging system and one could trivially add a system which writes all the loggings to file.
- Test framework, ... works.
- Prototype Histopyramid cpu implementation, made to understand the idea. Will create gpu implementation once the graphics library supports all the needed operations.
- Buildsystem, Fastbuild files are currently good enough. Linux support gets more attention when vulkan becomes more relevant.
