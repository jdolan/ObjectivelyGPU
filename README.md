[![Build](https://github.com/jdolan/ObjectivelyGPU/actions/workflows/build.yml/badge.svg)](https://github.com/jdolan/ObjectivelyGPU/actions/workflows/build.yml)
[![Zlib License](https://img.shields.io/badge/license-Zlib-brightgreen.svg)](https://opensource.org/licenses/Zlib)
![Beta](https://img.shields.io/badge/maturity-beta-yellow.svg)

# ObjectivelyGPU
Object oriented framework for modern cross-platform graphics with SDL3 and C.

Zlib [license](./COPYING).

## Dependencies

* [Objectively](https://github.com/jdolan/Objectively) >= 2.0.0
* [SDL3](https://github.com/libsdl-org/SDL) >= 3.2.0

## Shader Pipeline

ObjectivelyGPU uses a **GLSL → SPIR-V → MSL** pipeline for cross-platform shader support:

* **GLSL** (Vulkan 4.5) is the source language for all shaders
* **glslc** (from [shaderc](https://github.com/google/shaderc)) compiles GLSL to SPIR-V
* **shadercross** (from [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross)) converts SPIR-V to MSL, using SDL3-aware buffer assignments

Both SPIR-V and MSL blobs are versioned in the repository. Normal builds never invoke these tools. Run `make shaders` after editing a `.glsl` file to regenerate the blobs.

### Installing the tools

```sh
# glslc (part of shaderc)
brew install shaderc

# shadercross — build from source (SPIR-V → MSL path; no DXC needed)
git clone https://github.com/libsdl-org/SDL_shadercross
cd SDL_shadercross
git submodule update --init --recursive external/SPIRV-Cross external/SPIRV-Headers external/SPIRV-Tools
cmake -S . -B build \
  -DSDLSHADERCROSS_VENDORED=ON \
  -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF \
  -DSDLSHADERCROSS_CLI=ON \
  -DSDLSHADERCROSS_INSTALL=ON \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
sudo cmake --install build
```

> **Note:** `SDLSHADERCROSS_DXC=ON` and the `DirectXShaderCompiler` submodule are **not** required.
> DXC is only needed for HLSL input. The SPIR-V → MSL path used here works without it.

### Regenerating blobs

```sh
cd Examples
make shaders
```

This runs:
```
glslc -fshader-stage=<stage> Shader.glsl -o Shader.spv
shadercross Shader.spv -s SPIRV -d MSL -t <stage> -o Shader.msl
```

### GLSL binding layout (SDL3 GPU descriptor set convention)

| Stage    | Set | Binding | Usage                         |
|----------|-----|---------|-------------------------------|
| Vertex   | 0   | 0+      | Vertex samplers / storage bufs |
| Vertex   | 1   | 0+      | Vertex uniform buffers         |
| Fragment | 2   | 0+      | Fragment samplers / storage    |
| Fragment | 3   | 0+      | Fragment uniform buffers       |
| Compute  | 1   | 0+      | Read-write storage buffers     |
| Compute  | 2   | 0+      | Compute uniform buffers        |
