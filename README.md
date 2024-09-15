# Outline Triangulation

I wanted to create a library that triangulates outlines for rendering glyphs and svg

## Dependencies

### Linux

* wayland-client
* wayland-protocols (for xdg-shell)
* vulkan

### Windows

* Vulkan

### Common

* glm

## Usage

### Linux

```bash
./script/compile_shaders.sh
mkdir -p ./build
cd ./thirdparty/wlr-protocols
./prepare.sh
cd ../../build
cmake ..
make
cd ../bin
./outline-triangulation
```

### Windows

```powershell
./script/compile_shaders.bat
md ./winbuild -ea 0
cd ./winbuild
cmake .. -G "Visual Studio 17 2022"
```

Open the solution and hit the run button

## Status

Currently builds on both Linux and Windows and only renders single mesh
