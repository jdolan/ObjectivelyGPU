[![Build](https://github.com/jdolan/ObjectivelyGPU/actions/workflows/build.yml/badge.svg)](https://github.com/jdolan/ObjectivelyGPU/actions/workflows/build.yml)
[![Zlib License](https://img.shields.io/badge/license-Zlib-brightgreen.svg)](https://opensource.org/licenses/Zlib)
![Beta](https://img.shields.io/badge/maturity-beta-yellow.svg)

# ObjectivelyGPU
Object oriented graphics framework for SDL3 and C.

Zlib [license](./COPYING).

## About
ObjectivelyGPU is a cross-platform, object oriented graphics framework for the C programming language.
Built on [Objectively](https://github.com/jdolan/Objectively) and [SDL3](https://libsdl.org)'s GPU API, it
provides a clean, idiomatic C API for modern GPU programming — targeting Metal, Vulkan, and Direct3D 12
through a single interface.

## Features
 * **macOS, iOS, Windows, Linux & Android** cross-platform support via SDL3 (Metal, Direct3D 12, Vulkan)
 * **RenderDevice** owns the swapchain — drive frames with a simple `beginFrame` / `endFrame` loop
 * **Resource objects** with automatic lifecycle: Buffer, Texture, Sampler, Shader, GraphicsPipeline, ComputePipeline
 * **Typed passes**: RenderPass, ComputePass, CopyPass with command-lifecycle validation
 * **Framebuffer** with multiple render targets, depth, and MSAA with automatic resolve
 * **Shaders** in every format SDL3 supports — SPIR-V (Vulkan), MSL (Metal), DXIL (D3D12) — loaded by name with automatic per-backend selection; author them in any language and cross-compile with [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross), or bring your own blobs
 * **Mathlib**: vector, matrix, and quaternion math for 3D graphics

The verbose, error-prone handle juggling of a raw GPU API collapses into reference-counted objects and a
handful of methods. A whole frame — acquire the swapchain, clear, draw a multisampled scene, resolve, and
present — is just this:

```c
CommandBuffer *commands = $(renderDevice, beginFrame);
if (commands) {

  const SDL_FColor clearColor = { 0.1f, 0.1f, 0.2f, 1.f };
  const SDL_GPUColorTargetInfo color = $(framebuffer, colorTargetInfo, 0, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, &clearColor);
  const SDL_GPUDepthStencilTargetInfo depth = $(framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);

  RenderPass *pass = $(commands, beginRenderPass, &color, 1, &depth);
  $(pass, bindPipeline, pipeline);
  $(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = vertexBuffer->buffer }, 1);
  $(pass, drawPrimitives, 36, 1, 0, 0);
  pass = release(pass);

  $(renderDevice, endFrame);
}
```

`endFrame` resolves the multisampled `Framebuffer` and presents to the swapchain for you — no manual
resolve targets, store-op bookkeeping, or present plumbing.

Read the **[Guide](https://jdolan.github.io/ObjectivelyGPU/guide.html)** for the render device, resource
objects, framebuffers and MSAA, the typed passes, and shaders.

## API Documentation

Browse the [API Documentation](https://jdolan.github.io/ObjectivelyGPU/) to explore the library.

## Getting Started

### Dependencies

* [Objectively](https://github.com/jdolan/Objectively) >= 2.0.0
* [SDL3](https://github.com/libsdl-org/SDL) >= 3.2.0

### Building

```sh
autoreconf -i
./configure
make && sudo make install
```

## Examples & projects using ObjectivelyGPU

1. [Hello](Examples/Hello.c) renders a spinning, multisampled 3D cube.
1. [HelloCompute](Examples/HelloCompute.c) animates particles with a compute shader and draws them as points.
1. [ObjectivelyMVC](https://github.com/jdolan/ObjectivelyMVC) is a framework for modern game interfaces built on SDL3, Objectively and ObjectivelyGPU.
1. [Quetoo](https://github.com/jdolan/quetoo) is a free first-person shooter that uses Objectively, ObjectivelyGPU and ObjectivelyMVC extensively.

<p align="center">
  <img src="Documentation/Hello.gif" width="48%" alt="Hello — a spinning, multisampled 3D cube">
  <img src="Documentation/HelloCompute.gif" width="48%" alt="HelloCompute — particles animated by a compute shader">
</p>
