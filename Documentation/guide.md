ObjectivelyGPU User Guide {#guide}
=========================

A tour of drawing with ObjectivelyGPU.

[TOC]

## Object oriented GPU programming in C

ObjectivelyGPU is built on [Objectively](https://github.com/jdolan/Objectively), a lightweight object oriented framework for C. The raw SDL3 GPU API is a bag of opaque handles that you create, track, and destroy by hand; ObjectivelyGPU wraps them in reference-counted objects with strongly typed methods, so resources clean up after themselves and the API guides you toward correct usage.

Everything starts with a `RenderDevice`, created for the window you intend to draw to:

```c
RenderDevice *renderDevice = $(alloc(RenderDevice), initWithWindow, window, NULL);
```

The device claims the GPU, configures the swapchain for that window, and becomes the factory for every other resource.

## The render device owns the swapchain

ObjectivelyGPU draws to a `Framebuffer` and lets the `RenderDevice` manage the swapchain for you. Each frame is bracketed by `beginFrame` and `endFrame`:

```c
CommandBuffer *commands = $(renderDevice, beginFrame);
if (commands) {
  // ... record passes against `commands` ...
  $(renderDevice, endFrame);
}
```

`beginFrame` acquires the swapchain texture and returns a `CommandBuffer`, or `NULL` if the swapchain is unavailable this frame (e.g. the window is minimized) — just skip the frame. `endFrame` resolves the active multisampled `Framebuffer`, blits it to the swapchain, and submits. Applications that would rather render straight to the swapchain can ignore this loop entirely and drive the `CommandBuffer` themselves — `beginFrame` / `endFrame` are a convenience, not a requirement.

## Resource objects with automatic lifecycle

Buffers, textures, samplers, shaders, and pipelines are all objects, created through the device and released when you are done. Reference counting means `release` returns `NULL`, so the `foo = release(foo)` idiom keeps dangling pointers from biting you.

```c
Buffer *vertexBuffer = $(renderDevice, createBufferWithConstMem,
    SDL_GPU_BUFFERUSAGE_VERTEX, vertices, sizeof(vertices));

Shader *vertexShader = $(renderDevice, loadShader, "Hello.vert", &(SDL_GPUShaderCreateInfo) {
  .stage = SDL_GPU_SHADERSTAGE_VERTEX,
  .num_uniform_buffers = 1,
});

Shader *fragmentShader = $(renderDevice, loadShader, "Hello.frag", &(SDL_GPUShaderCreateInfo) {
  .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
});

// ... build a pipeline from the shaders, then ...

vertexShader = release(vertexShader);
fragmentShader = release(fragmentShader);
```

The underlying SDL handle is always reachable as a field (`vertexBuffer->buffer`, `shader->shader`) for the places SDL still wants one — you get encapsulation without losing access to the metal.

## Framebuffers, MSAA, and automatic resolve

A `Framebuffer` bundles one or more color targets, an optional depth target, and a sample count. Create one through the device with a `GPU_FramebufferCreateInfo`, then make it the device's active framebuffer:

```c
const SDL_GPUTextureFormat colorFormat = $(renderDevice, getSwapchainTextureFormat);

Framebuffer *framebuffer = $(renderDevice, createFramebuffer, &(GPU_FramebufferCreateInfo) {
  .size = MakeSize(w, h),
  .colorFormats = { colorFormat },
  .numColorTargets = 1,
  .depthFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
  .sampleCount = SDL_GPU_SAMPLECOUNT_4,
});

$(renderDevice, setFramebuffer, framebuffer);
```

When `sampleCount` is greater than one, the framebuffer allocates the resolve targets and promotes your `STORE` operations to `RESOLVE_AND_STORE` automatically. You ask for target descriptors with `colorTargetInfo` and `depthTargetInfo`; the MSAA bookkeeping is handled for you, and `endFrame` presents the resolved result.

## Typed passes with lifecycle validation

A `CommandBuffer` vends three kinds of pass — `RenderPass`, `ComputePass`, and `CopyPass` — each a distinct type exposing only the methods that are legal within it. The framework asserts on misuse (recording into a finished buffer, leaving a pass open, submitting twice), turning a class of silent GPU errors into immediate, located failures.

```c
const SDL_GPUColorTargetInfo color = $(framebuffer, colorTargetInfo, 0, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE, &clearColor);
const SDL_GPUDepthStencilTargetInfo depth = $(framebuffer, depthTargetInfo, SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);

RenderPass *pass = $(commands, beginRenderPass, &color, 1, &depth);
$(pass, bindPipeline, pipeline);
$(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = vertexBuffer->buffer }, 1);
$(pass, drawPrimitives, 36, 1, 0, 0);
pass = release(pass);
```

Per-frame uniform data goes through the command buffer — for example, pushing a model-view-projection matrix to vertex uniform slot 0:

```c
$(commands, pushVertexUniformData, 0, modelViewProjection.f, sizeof(modelViewProjection));
```

## Shaders in any language

ObjectivelyGPU loads compiled shaders in whatever format the active backend expects — SPIR-V for Vulkan, MSL for Metal, DXIL for Direct3D 12 — through the Objectively `Resource` system. `loadShader` looks the blob up by name and selects the right variant for the running backend, so your code names a shader once and runs everywhere.

How you *author* those shaders is entirely up to you. Write GLSL, HLSL, or Metal; hand-write the target language; or cross-compile a single source with [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross). ObjectivelyGPU has no opinion — it only consumes the compiled result.

## Examples

- [Hello](https://github.com/jdolan/ObjectivelyGPU/blob/main/Examples/Hello.c) — a spinning, multisampled 3D cube, start to finish: device, framebuffer, pipeline, and the frame loop.
- [HelloCompute](https://github.com/jdolan/ObjectivelyGPU/blob/main/Examples/HelloCompute.c) — a compute shader animates a particle system that is then drawn as points.
