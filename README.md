# Render Hardrware Interface (RHINO)
### Multiplatform RHI with Ray Tracing support

---

RHINO - is a modern render hardware interface that provides universal API for complex GPU computations.  
It is based on the bindless approach, which allows you to dynamically address a huge amount of resources directly during
the execution of program on the GPU side.

## Features:
- Multiplatform support :computer:
- Low-end API :rocket:
- Synchronisation primitives :hourglass: 
- Bindless approach :gem:
- Debug layer :hand:

## Supported APIs:
- Direct3D 12 :wrench:
- Vulkan 1.3 :construction:
- Apple Metal 3 :construction:

### Platform / API support table

|         API |         Windows         |     Linux      |     MacOS      |      iOS       |
|------------:|:-----------------------:|:--------------:|:--------------:|:--------------:|
| Direct3D 12 | :ballot_box_with_check: |      N/A       |      N/A       |      N/A       |
|  Vulkan 1.3 |     :construction:      | :construction: |      N/A       |      N/A       |
|     Metal 3 |           N/A           |      N/A       | :construction: | :construction: |

## Building
### Requirements:
#### Common:
- CMake 3.25 or newer | [download](https://cmake.org/download/)
- Vulkan SDK 1.3.261.1 or newer | [download](https://www.lunarg.com/vulkan-sdk/)
#### Windows
- Visual Studio latest version | [download](https://visualstudio.microsoft.com/vs/)
#### MacOS
- Xcode latest version | [download](https://apps.apple.com/ua/app/xcode/id497799835?mt=12)

### To use RHINO as dependency use:
```bash
git submodule add git@github.com:DanilAndreev/RHINO.git [path/to/install]
```
```cmake
add_subdirectory(path/to/install RHINO)
target_link_libraries(YourTarget PRIVATE RHINO)
```

### SCAR
> See README.md in ```/SCAR``` directory.