# open.mp SDK

## Tools

- [CMake 3.19+](https://cmake.org/)

## Tools on Windows

- [Visual Studio 2019+](https://www.visualstudio.com/)

## Sources

```bash
# With HTTPS:
git clone --recursive https://github.com/openmultiplayer/open.mp-sdk
# With SSH:
git clone --recursive git@github.com:openmultiplayer/open.mp-sdk
```

Note the use of the `--recursive` argument, because this repository contains submodules.

## Usage with cmake

```cmake
add_subdirectory(path_to_omp_sdk)

# Later in your cmake file
target_link_libraries(Server PUBLIC OMP-SDK)
```
