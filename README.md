# Attempt to learn Vulkan

This is a simple attempt to learn Vulkan based on [this website](https://vulkan-tutorial.com)

## How to build

1. Make sure cmake is installed.
2. Make sure Vulkan SDK and glfw3 and glm is installed. I used vcpkg to manage the packages. Make sure glslc is present
   and is in the PATH variable.
3. Create a directory call "build".
4. Run `cd build && cmake .. <options>`
5. If using Makefile or NMake, run `make vkTest` or `nmake vkTest`. Otherwise, with
   MSBuild, `msbuild <output sln file> -target:vkTest`

