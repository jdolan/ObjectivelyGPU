[![Build](https://github.com/jdolan/ObjectivelyGPU/actions/workflows/build.yml/badge.svg)](https://github.com/jdolan/ObjectivelyGPU/actions/workflows/build.yml)
[![Zlib License](https://img.shields.io/badge/license-Zlib-limegreen.svg)](https://opensource.org/licenses/Zlib)
![Beta](https://img.shields.io/badge/maturity-beta-yellow.svg)

# ObjectivelyGPU
Object oriented graphics framework for SDL3 and C.

Zlib [license](./COPYING).

## About
ObjectivelyGPU is a cross-platform object oriented graphics framework for the C programming language.
Built on [Objectively](https://github.com/jdolan/Objectively) and [SDL_gpu](https://libsdl.org), it provides a unified API for modern GPU programming with Metal, Vulkan, and Direct3D 12.

## Features
 * **Cross-platform** support for Android, iOS, macOS, Linux and Windows
 * **Resource objects** with automatic lifecycle — Buffer, Texture, Sampler, Shader, GraphicsPipeline, ComputePipeline and RenderDevice initialize and tear themselves down
 * **Typed passes**: RenderPass, ComputePass and CopyPass with command-lifecycle validation that catches mistakes upfront
 * **Framebuffer** abstracts multiple render targets, depth, and MSAA with automatic resolve — convenience without forfeiting the low-level API
 * **Shaders** loaded by name, with the formats supported by your platform
 * **Mathlib** — vector, matrix, and quaternion math for 3D graphics

## tl;dr
The verbose, error-prone handling of a raw GPU API collapses into reference-counted objects and a
handful of intuitive methods. 

Uploading an image to the GPU is a single call — the staging buffer and copy pass are handled for you:

```c
SDL_Surface *surface = IMG_Load("crate.png");
Texture *texture = $(device, createTextureFromSurface, surface, SDL_GPU_TEXTUREUSAGE_SAMPLER);
SDL_DestroySurface(surface);
```

And a whole frame — acquire the swapchain, clear, draw a multisampled scene, resolve, and present:

```c
CommandBuffer *commands = $(device, beginFrame);
if (commands) {

  const SDL_FColor clearColor = { 0.1f, 0.1f, 0.2f, 1.f };
  const SDL_GPUColorTargetInfo color = $(framebuffer, colorTargetInfo, 0, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, &clearColor);
  const SDL_GPUDepthStencilTargetInfo depth = $(framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);

  RenderPass *pass = $(commands, beginRenderPass, &color, 1, &depth);
  $(pass, bindPipeline, pipeline);
  $(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = vertexBuffer->buffer }, 1);
  $(pass, drawPrimitives, 36, 1, 0, 0);
  pass = release(pass);

  $(device, endFrame);
}
```

The `endFrame` method resolves the multisampled `Framebuffer` and presents to the swapchain for you. No manual
resolve targets, no store-op bookkeeping, no presentation plumbing.

## Getting Started

Check out the **[Installation](https://jdolan.github.io/ObjectivelyGPU/install.html)** page for dependencies, building, and linking instructions.

## User Guide

Peep the **[User Guide](https://jdolan.github.io/ObjectivelyGPU/guide.html)** to draw your first frame — the
render device, resource objects, framebuffers and MSAA, the typed passes, and shaders.

## API Documentation

Browse the [API Documentation](https://jdolan.github.io/ObjectivelyGPU/) to explore the library further.

## Project Showcase

1. [Hello](Examples/Hello.c) renders a spinning, multisampled 3D cube.
1. [HelloCompute](Examples/HelloCompute.c) animates particles with a compute shader and draws them as points.
1. [ObjectivelyMVC](https://github.com/jdolan/ObjectivelyMVC) is a framework for modern game interfaces built on SDL3, Objectively and ObjectivelyGPU.
1. [Quetoo](https://github.com/jdolan/quetoo) is a free first-person shooter that uses Objectively, ObjectivelyGPU and ObjectivelyMVC extensively.

<p align="center">
  <img src="Documentation/Hello.gif" width="48%" alt="Hello — a spinning, multisampled 3D cube">
  <img src="Documentation/HelloCompute.gif" width="48%" alt="HelloCompute — particles animated by a compute shader">
  <img src="Documentation/ObjectivelyMVC-Hello.gif" width="48%" alt="Hello ObjectivelyMVC - a simple in-game menu">
</p>
