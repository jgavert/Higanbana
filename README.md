# Faze
Name depicts that the projects goal isn't clear, hence its referencing mist.
It's a collection of libraries to fasten researching or prototyping of multithreaded and/or advanced graphics algorithms.

###Requirements:
- Windows 10
- Visual Studio 2015 Community and Windows SDK 10
- Vulkan SDK
- Fastbuild binary copied to root folder.
- Graphics card supporting feature level 12.0
- externals
  - Look at externals.bff for tips.
  - Oculus SDK 0.8
  - Vulkan Cpp api

## Origins for some of the algorithms and motivation:
- Lazy binary-splitting: a run-time adaptive work-stealing scheduler http://www.umiacs.umd.edu/publications/lazy-binary-splitting-run-time-adaptive-work-stealing-scheduler

### "Nice to have" features, haven't had time or interest.
- Audio
- Extend window and input support for windows.
- Linux support.

### In development
DirectX 12 is quite long way.
Starting vulkan.

## Needs refactorings
Tasksystem needs a proper nodegraph. Helps debugging and makes dependency handling easier.
Buildsystem needs more changes and linux could be added as a platform now that I have a null graphics backend.

### Not in active development, but updated when they break or don't fulfill perfectly the task.
- Math library, Basic but fullfills the needed tasks. Has some tests.
- InputBuffer, Waiting for windower implementation so that I can look at how to even get inputs on windows.
- EntitySystem, Pretty good. Has support for the tasksystem to do parallellized queries into it. Doesn't really bring performance benefits unless the workload is "significant" per unit. Should always help performance if tasksystem is used extensively.
- Internal messaging system which supports sending messages to any subsystem desired from anywhere. Used only where helps.
- "Print" macro that uses the messaging system and one could trivially add a system which writes all the loggings to file.
- Test framework, ... works.
- Prototype Histopyramid cpu implementation, made to understand the idea. Will create gpu implementation once the graphics library supports all the needed operations.
