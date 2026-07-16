ObjectivelyGPU
==============
Object oriented graphics framework for SDL3 and C.

[View on GitHub](https://github.com/jdolan/ObjectivelyGPU) — Zlib license.

## About

[ObjectivelyGPU](https://github.com/jdolan/ObjectivelyGPU) is a cross-platform object oriented graphics framework for the C programming language. Built on [Objectively](https://github.com/jdolan/Objectively) and [SDL3_gpu](https://libsdl.org), it provides a unified API for modern GPU programming with Metal, Vulkan, and Direct3D 12.

## Features

- **Cross-platform** support for Android, iOS, macOS, Linux and Windows
- **Resource objects** with automatic lifecycle — Buffer, Texture, Sampler, Shader, GraphicsPipeline ComputePipeline and RenderDevice initialize and tear themselves down
- **Typed passes**: RenderPass, ComputePass and CopyPass with command-lifecycle validation that catches mistakes upfront
- **Framebuffer** abstracts multiple render targets, depth, and MSAA with automatic resolve — convenience without forfeiting the low-level API
- **Shaders** loaded by name, with the formats supported by your platform
- **Mathlib** — vector, matrix, and quaternion math for 3D graphics

## tl;dr

The verbose, error-prone handling of a raw GPU API collapses into reference-counted objects and a handful of intuitive methods.

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

  const SDL_GPUColorTargetInfo color = $(framebuffer, colorTargetInfo, 0, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE);
  const SDL_GPUDepthStencilTargetInfo depth = $(framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE);

  RenderPass *pass = $(commands, beginRenderPass, &color, 1, &depth);
  $(pass, bindPipeline, pipeline);
  $(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = vertexBuffer->buffer }, 1);
  $(pass, drawPrimitives, 36, 1, 0, 0);
  pass = release(pass);

  $(device, endFrame);
}
```

The `endFrame` method resolves the multisampled `Framebuffer` and presents to the swapchain for you. No manual resolve targets, no store-op bookkeeping, no presentation plumbing.

## Getting Started

Consult the @subpage install for dependencies, building, and linking.

## User Guide

Consult the @subpage guide to draw your first frame — the render device, resource objects, framebuffers and MSAA, the typed passes, and shaders.

## Class Hierarchy

Browse the [Class Hierarchy](hierarchy.html) to navigate the full API.

## Project Showcase

1. [Hello](Examples/Hello.c) renders a spinning, multisampled 3D cube.
1. [HelloCompute](Examples/HelloCompute.c) animates particles with a compute shader and draws them as points.
1. [ObjectivelyMVC](https://github.com/jdolan/ObjectivelyMVC) is a framework for modern game interfaces built on SDL3, Objectively and ObjectivelyGPU.
1. [Quetoo](https://github.com/jdolan/quetoo) is a free first-person shooter that uses Objectively, ObjectivelyGPU and ObjectivelyMVC extensively.

![Hello — a spinning, multisampled 3D cube](Hello.gif) 
![HelloCompute — particles animated by a compute shader](HelloCompute.gif)
![HelloMVC — ObjectivelyMVC](ObjectivelyMVC-Hello.gif)
