# Faze
Name depicts that the projects goal isn't clear, hence its referencing mist.
It's a collection of libraries to fasten researching or prototyping of multithreaded and/or advanced graphics algorithms.

### Current situation:
Working in another branch and this branch is thrown away practically.
so...
Switched buildsystem to bazel, working for what it's worth only on windows for now.
Brought DX12 down to the level of Vulkan and had a triangle on screen before big int handle refactoring.
Currently got api design down for higher level user code.
Working on getting lower level back to working with handles and multigpu.
  - working towards clear screen -> present loop
    - doesn't work yet. Handles and other design changes "broke" things.
  - after that, triangle
    - kind of have all the code for it... just need to figure out few things with new synchronization design that doesn't exist yet.
      ... I could do another set of platform specific solvers, but that's a pain to upkeep... no thanks.
    - at this point I will probably make "bazel" the main branch, next update will happen then.
