ObjectivelyGPU {#index}
===============

[ObjectivelyGPU](https://github.com/jdolan/ObjectivelyGPU) is an object-oriented wrapper around [SDL3](https://libsdl.org)'s GPU API for [GNU C](https://www.gnu.org/software/gnu-c-manual/). It builds on [Objectively](https://github.com/jdolan/Objectively) to deliver a clean, idiomatic C API for modern GPU programming.

## Features

- **RenderDevice** — device creation, buffer/texture/sampler allocation, and shader pipeline compilation
- **CommandBuffer** — render, compute, and copy command recording
- **RenderPass**, **ComputePass**, **CopyPass** — typed command encoder wrappers
- Shader compilation via [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross) (HLSL → SPIR-V / MSL)
- macOS and iOS (Metal), Windows (Direct3D 12), Linux (Vulkan) via SDL3 backends

## API

Browse the [class hierarchy](hierarchy.html) to navigate the API.

## Building

```sh
autoreconf -i
./configure
make && sudo make install
```
