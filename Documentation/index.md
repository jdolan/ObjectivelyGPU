ObjectivelyGPU
==============
Object oriented graphics framework for SDL3 and C.

[View on GitHub](https://github.com/jdolan/ObjectivelyGPU) — Zlib license.

## About

[ObjectivelyGPU](https://github.com/jdolan/ObjectivelyGPU) is a cross-platform, object oriented graphics framework for the C programming language. Built on [Objectively](https://github.com/jdolan/Objectively) and [SDL3](https://libsdl.org)'s GPU API, it provides a clean, idiomatic C API for modern GPU programming across Metal, Vulkan, and Direct3D 12.

## Features

- **macOS, iOS, Windows, Linux & Android** cross-platform support via SDL3 (Metal, Direct3D 12, Vulkan)
- **RenderDevice** owns the swapchain — drive frames with a simple `beginFrame` / `endFrame` loop
- **Resource objects** with automatic lifecycle: Buffer, Texture, Sampler, Shader, GraphicsPipeline, ComputePipeline
- **Typed passes**: RenderPass, ComputePass, CopyPass with command-lifecycle validation
- **Framebuffer** with multiple render targets, depth, and MSAA with automatic resolve
- **Shaders** in every format SDL3 supports (SPIR-V, MSL, DXIL), loaded by name with automatic per-backend selection
- **Mathlib**: vector, matrix, and quaternion math for 3D graphics

## Class Hierarchy

Browse the [class hierarchy](hierarchy.html) to navigate the full API.

## Getting Started

```sh
autoreconf -i
./configure
make && sudo make install
```

See the @subpage guide for the render device, resource objects, framebuffers and MSAA, the typed passes, and shaders.

## Examples

![Hello — a spinning, multisampled 3D cube](Hello.gif)

![HelloCompute — particles animated by a compute shader](HelloCompute.gif)
