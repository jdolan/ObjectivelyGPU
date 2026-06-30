ObjectivelyGPU
==============
Object oriented graphics framework for SDL3 and C.

Zlib license.

## About

[ObjectivelyGPU](https://github.com/jdolan/ObjectivelyGPU) is a cross-platform, object oriented graphics framework for the C programming language. Built on [Objectively](https://github.com/jdolan/Objectively) and [SDL3](https://libsdl.org)'s GPU API, it provides a clean, idiomatic C API for modern GPU programming across Metal, Vulkan, and Direct3D 12.

## Features

- **macOS, iOS, Windows, Linux & Android** cross-platform support via SDL3 (Metal, Direct3D 12, Vulkan)
- **RenderDevice** with a `beginFrame` / `endFrame` loop over the swapchain
- **Resource objects**: Buffer, Texture, Sampler, Shader, GraphicsPipeline, ComputePipeline
- **Typed passes**: RenderPass, ComputePass, CopyPass with command-lifecycle validation
- **Framebuffer** with multiple render targets, depth, and MSAA with automatic resolve
- **Shaders** authored in GLSL and loaded via the Objectively Resource system
- **Mathlib**: vector, matrix, and quaternion math for 3D graphics

## Class Hierarchy

Browse the [class hierarchy](hierarchy.html) to navigate the full API.

## Getting Started

```sh
autoreconf -i
./configure
make && sudo make install
```

See the [Hello](https://github.com/jdolan/ObjectivelyGPU/blob/main/Examples/Hello.c) and [HelloCompute](https://github.com/jdolan/ObjectivelyGPU/blob/main/Examples/HelloCompute.c) examples to draw your first frame.

## Examples

![Hello — a spinning, multisampled 3D cube](Hello.gif)

![HelloCompute — particles animated by a compute shader](HelloCompute.gif)
