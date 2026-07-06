/*
 * ObjectivelyGPU: Object oriented graphics framework for SDL3 and C.
 * Copyright (C) 2026 Jay Dolan <jay@jaydolan.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source distribution.
 */

#pragma once

#include <stdlib.h>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_surface.h>

#include <Objectively/Data.h>
#include <Objectively/Object.h>

#include <ObjectivelyGPU/CommandBuffer.h>
#include <ObjectivelyGPU/QueryPool.h>
#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief RenderDevice owns `SDL_gpu` infrastructure for a window and frame loop.
 */

typedef struct Buffer Buffer;
typedef struct CommandBuffer CommandBuffer;
typedef struct ComputePipeline ComputePipeline;
typedef struct Framebuffer Framebuffer;
typedef struct GPU_FramebufferCreateInfo GPU_FramebufferCreateInfo;
typedef struct GraphicsPipeline GraphicsPipeline;
typedef struct QueryPool QueryPool;
typedef struct RenderDevice RenderDevice;
typedef struct RenderDeviceInterface RenderDeviceInterface;
typedef struct Sampler Sampler;
typedef struct Shader Shader;
typedef struct Texture Texture;

/**
 * @brief The RenderDevice encapsulates an `SDL_GPUDevice` and provides methods for allocating
 * GPU resources, managing compute, copy and render passes, and .
 *
 * @extends Object
 */
struct RenderDevice {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  RenderDeviceInterface *interface;

  /**
   * @brief The `SDL_GPUDevice`.
   */
  SDL_GPUDevice *device;

  /**
   * @brief The `SDL_Window` associated with @c device.
   */
  SDL_Window *window;
  
  /**
   * The bitmask of supported shader formats.
   */
  SDL_GPUShaderFormat shaderFormats;

  /**
   * @brief The anisotropic filtering level applied by `createSamplerLinearRepeat`
   * and `createSamplerLinearClamp`. Defaults to `0` (anisotropic filtering
   * disabled). Assign directly; only affects Samplers created afterward, since
   * SDL_gpu Samplers are immutable once created.
   */
  float maxAnisotropy;

  /**
   * @brief The present-target Framebuffer driven by `beginFrame`/`endFrame`, or `NULL`.
   * @details Set via `setFramebuffer` (retained). `beginFrame` resizes it to the
   *   swapchain each frame and `endFrame` blits its resolved color to the swapchain.
   */
  Framebuffer *framebuffer;

  /**
   * @brief The command buffer in flight between `beginFrame` and `endFrame`, or `NULL`.
   * @details Owned by the device for the duration of the frame; `beginFrame` returns a
   *   borrowed view of it and `endFrame` submits and releases it.
   * @private
   */
  CommandBuffer *commands;

  /**
   * @brief The swapchain texture acquired for the current frame.
   * @details Valid only between `beginFrame` and `endFrame`.
   * @private
   */
  SwapchainTexture swapchain;
  
  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The RenderDevice interface.
 */
struct RenderDeviceInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn CommandBuffer *RenderDevice::acquireCommandBuffer(const RenderDevice *self)
   * @brief Acquires a new CommandBuffer for recording GPU commands this frame.
   * @details Wraps `SDL_AcquireGPUCommandBuffer`. The returned CommandBuffer is
   *   owned by the caller and must be submitted via `submit` or `submitAndFence`
   *   and then released with `release`.
   * @param self The RenderDevice.
   * @return A new, retained CommandBuffer. GPU_Asserts on failure.
   * @memberof RenderDevice
   */
  CommandBuffer *(*acquireCommandBuffer)(const RenderDevice *self);

  /**
   * @fn CommandBuffer *RenderDevice::beginFrame(RenderDevice *self)
   * @brief Begins a frame: acquires a command buffer and the swapchain, and prepares the framebuffer.
   * @details Convenience over the manual acquire→wait-swapchain→resize boilerplate.
   *   Acquires a CommandBuffer, blocks for the swapchain texture, resizes the framebuffer
   *   set via `setFramebuffer` to the swapchain dimensions, and returns the command buffer
   *   so the caller can record passes into `framebuffer`. Returns `NULL` when the swapchain
   *   is unavailable (e.g. the window is minimised); the command buffer is cancelled and the
   *   frame should be skipped. Pair every non-NULL return with `endFrame`. The returned
   *   CommandBuffer is owned by the device; do not release it. Applications that render
   *   directly to the swapchain can ignore `beginFrame`/`endFrame` and drive the command
   *   buffer themselves.
   * @param self The RenderDevice.
   * @return The frame's CommandBuffer (borrowed), or `NULL` to skip the frame.
   * @memberof RenderDevice
   */
  CommandBuffer *(*beginFrame)(RenderDevice *self);

  /**
   * @fn Buffer *RenderDevice::createBuffer(RenderDevice *self, const SDL_GPUBufferCreateInfo *info)
   * @brief Creates a Buffer (vertex, index, indirect, storage, etc.).
   * @details Convenience factory for `Buffer::initWithDevice`. The buffer's contents
   *   are uninitialised; use `Buffer::upload` to populate it.
   * @param self The RenderDevice.
   * @param info Buffer creation parameters (usage flags, size).
   * @return A new, retained Buffer. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Buffer *(*createBuffer)(RenderDevice *self, const SDL_GPUBufferCreateInfo *info);

  /**
   * @fn Buffer *RenderDevice::createBufferWithConstMem(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size)
   * @brief Creates a Buffer and uploads @p mem in a single call.
   * @details Convenience factory for `Buffer::initWithConstMem`. The returned buffer
   *   is immediately GPU-resident.
   * @param self The RenderDevice.
   * @param usage Buffer usage flags (e.g. `SDL_GPU_BUFFERUSAGE_VERTEX`).
   * @param mem CPU pointer to the source data. Must not be NULL.
   * @param size Number of bytes to allocate and upload.
   * @return A new, retained Buffer populated with @p mem. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Buffer *(*createBufferWithConstMem)(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size);

  /**
   * @fn Buffer *RenderDevice::createBufferWithData(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const Data *data)
   * @brief Creates a Buffer and uploads the contents of an Objectively `Data` object.
   * @details Convenience factory for `Buffer::initWithData`, for data loaded via the
   *   Objectively Resource system.
   * @param self The RenderDevice.
   * @param usage Buffer usage flags (e.g. `SDL_GPU_BUFFERUSAGE_VERTEX`).
   * @param data Source data object. Must not be NULL.
   * @return A new, retained Buffer populated with @p data. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Buffer *(*createBufferWithData)(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const Data *data);

  /**
   * @fn ComputePipeline *RenderDevice::createComputePipeline(RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info)
   * @brief Creates a ComputePipeline from a compiled compute shader.
   * @details Convenience factory for `ComputePipeline::initWithDevice`. Prefer
   *   `loadComputePipeline` to load a compiled blob from the Resource system.
   * @param self The RenderDevice.
   * @param info Compute pipeline creation parameters, including the shader.
   * @return A new, retained ComputePipeline. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  ComputePipeline *(*createComputePipeline)(RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info);

  /**
   * @fn Framebuffer *RenderDevice::createFramebuffer(RenderDevice *self, const GPU_FramebufferCreateInfo *info)
   * @brief Creates a Framebuffer aggregating color/depth attachments (and an MSAA resolve target).
   * @details Convenience factory for `Framebuffer::initWithDevice`. Pair with
   *   `setFramebuffer` to drive it via `beginFrame`/`endFrame`.
   * @param self The RenderDevice.
   * @param info Framebuffer creation parameters (size, formats, sample count).
   * @return A new, retained Framebuffer. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Framebuffer *(*createFramebuffer)(RenderDevice *self, const GPU_FramebufferCreateInfo *info);

  /**
   * @fn GraphicsPipeline *RenderDevice::createGraphicsPipeline(RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info)
   * @brief Creates a GraphicsPipeline from vertex and fragment shaders plus render state.
   * @details Convenience factory for `GraphicsPipeline::initWithDevice`.
   * @param self The RenderDevice.
   * @param info Graphics pipeline creation parameters (shaders, vertex layout,
   *   blend state, rasteriser state, depth/stencil state, colour target formats).
   * @return A new, retained GraphicsPipeline. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  GraphicsPipeline *(*createGraphicsPipeline)(RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info);

  /**
   * @fn QueryPool *RenderDevice::createQueryPool(RenderDevice *self, const SDL_GPUQueryPoolCreateInfo *info)
   * @brief Creates a QueryPool for occlusion or timestamp queries.
   * @details Convenience factory for `QueryPool::initWithDevice`. Always
   *   succeeds, even when occlusion queries are unsupported by the linked
   *   SDL3 -- see `QueryPool`.
   * @param self The RenderDevice.
   * @param info Query pool creation parameters (query type, query count).
   * @return A new, retained QueryPool. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  QueryPool *(*createQueryPool)(RenderDevice *self, const SDL_GPUQueryPoolCreateInfo *info);

  /**
   * @fn Sampler *RenderDevice::createSampler(RenderDevice *self, const SDL_GPUSamplerCreateInfo *info)
   * @brief Creates a Sampler describing filter and address modes.
   * @details Convenience factory for `Sampler::initWithDevice`. For the common
   *   filtering presets below, prefer `createSamplerLinearRepeat` et al.
   * @param self The RenderDevice.
   * @param info Sampler creation parameters (min/mag filter, mip mode, address modes, anisotropy, etc.).
   * @return A new, retained Sampler. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Sampler *(*createSampler)(RenderDevice *self, const SDL_GPUSamplerCreateInfo *info);

  /**
   * @fn Sampler *RenderDevice::createSamplerLinearRepeat(RenderDevice *self)
   * @brief Creates a trilinear, wrapping Sampler -- the common preset for tiled surface textures.
   * @details Linear min/mag filter, linear mipmap mode, `REPEAT` addressing on all
   *   three axes. Anisotropic filtering is enabled when `self->maxAnisotropy` is
   *   greater than `0`.
   * @param self The RenderDevice.
   * @return A new, retained Sampler. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Sampler *(*createSamplerLinearRepeat)(RenderDevice *self);

  /**
   * @fn Sampler *RenderDevice::createSamplerLinearClamp(RenderDevice *self)
   * @brief Creates a trilinear, edge-clamped Sampler -- the common preset for
   *   continuously-sampled volumes and non-tiling images (ambient/voxel volumes,
   *   cubemaps, post-process targets, UI images).
   * @details Linear min/mag filter, linear mipmap mode, `CLAMP_TO_EDGE` addressing
   *   on all three axes. Anisotropic filtering is enabled when `self->maxAnisotropy`
   *   is greater than `0`.
   * @param self The RenderDevice.
   * @return A new, retained Sampler. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Sampler *(*createSamplerLinearClamp)(RenderDevice *self);

  /**
   * @fn Sampler *RenderDevice::createSamplerNearestClamp(RenderDevice *self)
   * @brief Creates a point-filtered, edge-clamped Sampler -- the common preset
   *   for data textures fetched by integer coordinate (voxel light data,
   *   depth-attachment copies) rather than sampled for appearance.
   * @details Nearest min/mag filter, nearest mipmap mode, `CLAMP_TO_EDGE`
   *   addressing on all three axes.
   * @param self The RenderDevice.
   * @return A new, retained Sampler. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Sampler *(*createSamplerNearestClamp)(RenderDevice *self);

  /**
   * @fn Sampler *RenderDevice::createSamplerShadowCompare(RenderDevice *self)
   * @brief Creates a hardware-PCF comparison Sampler for shadow map sampling.
   * @details Linear min/mag filter, nearest mipmap mode, `CLAMP_TO_EDGE`
   *   addressing, depth comparison enabled with `SDL_GPU_COMPAREOP_LESS_OR_EQUAL`.
   * @param self The RenderDevice.
   * @return A new, retained Sampler. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Sampler *(*createSamplerShadowCompare)(RenderDevice *self);

  /**
   * @fn Shader *RenderDevice::createShader(RenderDevice *self, const SDL_GPUShaderCreateInfo *info)
   * @brief Creates a Shader from a fully-filled `SDL_GPUShaderCreateInfo`.
   * @details Convenience factory for `Shader::initWithDevice`. All fields of @p info,
   *   including `code`, `code_size`, and `format`, must be set by the caller. Prefer
   *   `loadShader` to load compiled blobs from the Resource system with automatic
   *   format selection.
   * @param self The RenderDevice.
   * @param info Shader creation parameters with all fields populated.
   * @return A new, retained Shader. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Shader *(*createShader)(RenderDevice *self, const SDL_GPUShaderCreateInfo *info);

  /**
   * @fn Texture *RenderDevice::createTexture(RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels)
   * @brief Creates a Texture, optionally uploading initial pixel data.
   * @details Convenience factory for `Texture::initWithDevice`. When @p pixels is
   *   non-NULL, the pixel data is uploaded immediately so the texture is
   *   GPU-resident before returning.
   * @param self The RenderDevice.
   * @param info Texture creation parameters (format, type, dimensions, usage, etc.).
   * @param pixels Initial pixel data to upload, or `NULL` to leave the texture uninitialised.
   * @return A new, retained Texture. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Texture *(*createTexture)(RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels);

  /**
   * @fn Texture *RenderDevice::createTextureFromSurface(RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage, bool generateMipmaps)
   * @brief Creates a Texture from an `SDL_Surface`, uploading pixel data immediately.
   * @details Convenience factory for `Texture::initWithSurface`. Converts @p surface
   *   to `SDL_PIXELFORMAT_RGBA32` if needed and uploads it as
   *   `SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM`. The surface is not modified or freed.
   * @param self The RenderDevice.
   * @param surface The source surface. Must not be NULL.
   * @param usage Texture usage flags (e.g. `SDL_GPU_TEXTUREUSAGE_SAMPLER`).
   * @param mipmaps If true, a full mip chain is allocated and generated; see `Texture::initWithSurface`.
   * @return A new, retained Texture. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Texture *(*createTextureFromSurface)(RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage, bool generateMipmaps);

  /**
   * @fn Texture *RenderDevice::createSolidColorTexture(RenderDevice *self, SDL_GPUTextureType type, Uint32 layerCount, Uint32 rgba)
   * @brief Creates a small, solid-color Texture -- the common "placeholder" pattern
   *   for a sampler slot a shader declares but a given draw never actually reads.
   * @details `type` may be `SDL_GPU_TEXTURETYPE_2D` (@p layerCount ignored, treated
   *   as 1) or `SDL_GPU_TEXTURETYPE_CUBE` (@p layerCount must be 6); every texel of
   *   every layer is filled with @p rgba. Always 1x1 per layer/face,
   *   `SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM`, one mip level, `SAMPLER` usage only.
   * @param self The RenderDevice.
   * @param type `SDL_GPU_TEXTURETYPE_2D` or `SDL_GPU_TEXTURETYPE_CUBE`.
   * @param layerCount The layer/face count (6 for `_CUBE`, otherwise ignored).
   * @param rgba The fill color, packed as `0xAABBGGRR` (i.e. red in the lowest byte).
   * @return A new, retained Texture. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Texture *(*createSolidColorTexture)(RenderDevice *self, SDL_GPUTextureType type, Uint32 layerCount, Uint32 rgba);

  /**
   * @fn SDL_GPUTransferBuffer *RenderDevice::createTransferBuffer(const RenderDevice *self, const SDL_GPUTransferBufferCreateInfo *info)
   * @brief Creates a CPU-accessible transfer buffer for staging data to or from the GPU.
   * @param self The RenderDevice.
   * @param info Transfer buffer creation parameters (usage: upload or download, size).
   * @return A new `SDL_GPUTransferBuffer`. GPU_Asserts on failure. Release with `releaseTransferBuffer`.
   * @memberof RenderDevice
   */
  SDL_GPUTransferBuffer *(*createTransferBuffer)(const RenderDevice *self, const SDL_GPUTransferBufferCreateInfo *info);

  /**
   * @fn void RenderDevice::endFrame(RenderDevice *self)
   * @brief Ends the frame begun by `beginFrame`: presents the framebuffer and submits.
   * @details Blits the framebuffer's resolved color (`Framebuffer::resolveColorTexture`,
   *   the resolve target when multisampled) into the acquired swapchain texture, submits
   *   the frame's command buffer, and releases it. Must be paired with a non-NULL
   *   `beginFrame` return.
   * @param self The RenderDevice.
   * @memberof RenderDevice
   */
  void (*endFrame)(RenderDevice *self);

  /**
   * @fn SDL_GPUTextureFormat RenderDevice::getSwapchainTextureFormat(const RenderDevice *self)
   * @brief Returns the pixel format of this device's swapchain.
   * @details Useful for configuring render-pass colour target formats or creating
   *   pipelines that write to the swapchain. The device must have a window claimed.
   * @param self The RenderDevice.
   * @return The `SDL_GPUTextureFormat` of the window's swapchain.
   * @memberof RenderDevice
   */
  SDL_GPUTextureFormat (*getSwapchainTextureFormat)(const RenderDevice *self);

  /**
   * @fn RenderDevice *RenderDevice::init(RenderDevice *self)
   * @brief Initialises a RenderDevice, creating an `SDL_GPUDevice` with no window claimed.
   * @details Creates the device supporting MSL, SPIR-V, and DXIL shader formats.
   *   Use `setWindow` or `initWithWindow` to associate a window after init.
   * @param self The RenderDevice.
   * @return The initialised RenderDevice, or `NULL` on failure.
   * @memberof RenderDevice
   */
  RenderDevice *(*init)(RenderDevice *self);

  /**
   * @fn RenderDevice *RenderDevice::initWithWindow(RenderDevice *self, SDL_Window *window)
   * @brief Initialises a RenderDevice and immediately claims @p window for rendering.
   * @param self The RenderDevice.
   * @param window The `SDL_Window` to claim for this device.
   * @return The initialised RenderDevice, or `NULL` on failure.
   * @memberof RenderDevice
   */
  RenderDevice *(*initWithWindow)(RenderDevice *self, SDL_Window *window);

  /**
   * @fn Shader *RenderDevice::loadShader(RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info)
   * @brief Loads a shader blob via the Resource system and creates a Shader.
   * @details Appends each supported format's extension to @c name and resolves it via
   *   Objectively's ResourceProvider chain:
   *   - Metal (macOS/iOS): `.metallib`, then `.metal`
   *   - Vulkan (Linux/Android): `.spv`
   *   - D3D12 (Windows): `.dxil`
   *   The caller fills in @c stage, @c entrypoint, and binding counts in @c info.
   *   @c code, @c code_size, @c format, and a default @c entrypoint are filled in for you,
   *   then the completed info is passed to `Shader::initWithDevice`.
   *   Shader blobs are produced offline by @c sdl-shadercross.
   * @param self The RenderDevice.
   * @param name Shader base name without extension, e.g. @c "shaders/Renderer.vert".
   * @param info Shader creation parameters. @c code, @c code_size, and @c format
   *   are ignored; all other fields must be set by the caller.
   * @return A new, retained Shader. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  Shader *(*loadShader)(RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info);

  /**
  * @fn ComputePipeline *RenderDevice::loadComputePipeline(RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
  * @brief Loads a compiled compute shader blob via the Resource system and creates a ComputePipeline.
  * @details Parallel to `loadShader` for compute stages. Appends each supported format's
  *   extension to @c name and resolves it via Objectively's ResourceProvider chain:
  *   - Metal (macOS/iOS): `.metallib`, then `.metal`
  *   - Vulkan (Linux/Android): `.spv`
  *   - D3D12 (Windows): `.dxil`
  *   The caller fills in @c entrypoint, thread counts, and binding counts in @c info.
  *   @c code, @c code_size, @c format, and a default @c entrypoint are filled in for you,
  *   then the completed info is passed to `ComputePipeline::initWithDevice`.
  *   Shader blobs are produced offline by @c sdl-shadercross.
  * @param self The RenderDevice.
  * @param name Shader base name without extension, e.g. @c "HelloCompute.comp".
  * @param info Compute pipeline creation parameters. @c code, @c code_size, and
  *   @c format are ignored; all other fields must be set by the caller.
  * @return A new, retained ComputePipeline. GPU_Asserts on failure. Free with `release`.
  * @memberof RenderDevice
  */
  ComputePipeline *(*loadComputePipeline)(RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info);

  /**
   * @fn GraphicsPipeline *RenderDevice::loadGraphicsPipeline(RenderDevice *self, const char *vertexShaderName, const SDL_GPUShaderCreateInfo *vertexShaderInfo, const char *fragmentShaderName, const SDL_GPUShaderCreateInfo *fragmentShaderInfo, SDL_GPUGraphicsPipelineCreateInfo *info)
   * @brief Loads a vertex/fragment shader pair and creates a GraphicsPipeline from them.
   * @details Convenience over the load-both/assign/create/release-both boilerplate every
   *   graphics pipeline needs: calls `loadShader` for @p vertexShaderName and
   *   @p fragmentShaderName, assigns the results to @p info's `vertex_shader`/
   *   `fragment_shader` fields, calls `createGraphicsPipeline`, then releases both
   *   Shaders (they're retained by the pipeline internally; the caller never sees
   *   or owns them). @p info must otherwise be fully populated (vertex layout,
   *   blend/rasterizer/depth-stencil state, colour target formats) by the caller.
   * @param self The RenderDevice.
   * @param vertexShaderName Vertex shader base name, e.g. @c "shaders/bsp_vs".
   * @param vertexShaderInfo Vertex shader creation parameters (see `loadShader`).
   * @param fragmentShaderName Fragment shader base name, e.g. @c "shaders/bsp_fs".
   * @param fragmentShaderInfo Fragment shader creation parameters (see `loadShader`).
   * @param info Graphics pipeline creation parameters; `vertex_shader`/`fragment_shader`
   *   are overwritten, all other fields must already be set.
   * @return A new, retained GraphicsPipeline. GPU_Asserts on failure. Free with `release`.
   * @memberof RenderDevice
   */
  GraphicsPipeline *(*loadGraphicsPipeline)(RenderDevice *self,
                                             const char *vertexShaderName, const SDL_GPUShaderCreateInfo *vertexShaderInfo,
                                             const char *fragmentShaderName, const SDL_GPUShaderCreateInfo *fragmentShaderInfo,
                                             SDL_GPUGraphicsPipelineCreateInfo *info);

  /**
  * @fn void *RenderDevice::mapTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf, bool cycle)
   * @brief Maps a transfer buffer into CPU-accessible memory for reading or writing.
   * @details Pass `cycle = true` to let the driver use a fresh buffer allocation
   *   (avoiding a pipeline stall) when the buffer is already in use by the GPU.
   *   Call `unmapTransferBuffer` when done; the mapping must not be used after that.
   * @param self The RenderDevice.
   * @param tbuf The transfer buffer to map.
   * @param cycle If `true`, the driver may cycle to a new backing allocation.
   * @return A CPU pointer to the mapped region. GPU_Asserts on failure.
   * @memberof RenderDevice
   */
  void *(*mapTransferBuffer)(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf, bool cycle);

  /**
   * @fn bool RenderDevice::queryFence(const RenderDevice *self, SDL_GPUFence *fence)
   * @brief Non-blocking query of whether a GPU fence has been signaled.
   * @param self The RenderDevice.
   * @param fence The fence to query.
   * @return `true` if the fence is signaled (GPU work completed); `false` otherwise.
   * @memberof RenderDevice
   */
  bool (*queryFence)(const RenderDevice *self, SDL_GPUFence *fence);

  /**
   * @fn void RenderDevice::releaseFence(const RenderDevice *self, SDL_GPUFence *fence)
   * @brief Releases a fence returned by `submitAndFence`. Null-safe.
   * @param self The RenderDevice.
   * @param fence The fence to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseFence)(const RenderDevice *self, SDL_GPUFence *fence);

  /**
   * @fn void RenderDevice::releaseTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf)
   * @brief Releases a transfer buffer created by `createTransferBuffer`. Null-safe.
   * @param self The RenderDevice.
   * @param tbuf The transfer buffer to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseTransferBuffer)(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf);
  
  /**
   * @fn bool RenderDevice::setAllowedFramesInFlight(const RenderDevice *self, Uint32 allowed)
   * @brief Sets the maximum number of GPU frames that may be in flight concurrently.
   * @details Lower values reduce latency; higher values improve throughput. The
   *   default is determined by SDL. Takes effect on the next frame.
   * @param self The RenderDevice.
   * @param allowed Maximum concurrent in-flight frames (1–3 are typical values).
   * @return `true` on success, `false` if the value is unsupported.
   * @memberof RenderDevice
   */
  bool (*setAllowedFramesInFlight)(const RenderDevice *self, Uint32 allowed);

  /**
   * @fn void RenderDevice::setFramebuffer(RenderDevice *self, Framebuffer *framebuffer)
   * @brief Sets the present-target Framebuffer driven by `beginFrame`/`endFrame`.
   * @details The device retains @p framebuffer and releases any previously set one.
   *   Construct the Framebuffer with the desired color/depth formats and MSAA sample
   *   count; `beginFrame` resizes it to the swapchain each frame. Pass `NULL` to clear.
   * @param self The RenderDevice.
   * @param framebuffer The Framebuffer to present each frame, or `NULL`.
   * @memberof RenderDevice
   */
  void (*setFramebuffer)(RenderDevice *self, Framebuffer *framebuffer);

  /**
   * @fn bool RenderDevice::setSwapchainParameters(const RenderDevice *self, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode)
   * @brief Configures swapchain composition and present mode for this device's window.
   * @details Use `supportsSwapchainComposition` and `supportsPresentMode`
   *   to guard against unsupported combinations before calling this.
   * @param self The RenderDevice.
   * @param composition Colour space / HDR composition mode.
   * @param mode Presentation mode (vsync, mailbox, immediate, etc.).
   * @return `true` on success, `false` if the combination is unsupported.
   * @memberof RenderDevice
   */
  bool (*setSwapchainParameters)(const RenderDevice *self, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode);

  /**
   * @fn void RenderDevice::setWindow(RenderDevice *self, SDL_Window *window)
   * @brief Claims @p window for this device, releasing any previously claimed window.
   * @details If a different window is already claimed it is released via
   *   `SDL_ReleaseWindowFromGPUDevice` first. Passing `NULL` releases the current
   *   window without claiming a new one.
   * @param self The RenderDevice.
   * @param window The `SDL_Window` to claim, or `NULL` to release the current window.
   * @memberof RenderDevice
   */
  void (*setWindow)(RenderDevice *self, SDL_Window *window);

  /**
   * @fn bool RenderDevice::supportsPresentMode(const RenderDevice *self, SDL_GPUPresentMode mode)
   * @brief Queries whether this device's swapchain supports the given present mode.
   * @param self The RenderDevice.
   * @param mode The present mode to test.
   * @return `true` if the present mode is supported.
   * @memberof RenderDevice
   */
  bool (*supportsPresentMode)(const RenderDevice *self, SDL_GPUPresentMode mode);

  /**
   * @fn bool RenderDevice::supportsSwapchainComposition(const RenderDevice *self, SDL_GPUSwapchainComposition composition)
   * @brief Queries whether this device's swapchain supports the given composition (colour space / HDR mode).
   * @param self The RenderDevice.
   * @param composition The swapchain composition to test.
   * @return `true` if the composition is supported.
   * @memberof RenderDevice
   */
  bool (*supportsSwapchainComposition)(const RenderDevice *self, SDL_GPUSwapchainComposition composition);

  /**
   * @fn bool RenderDevice::textureSupportsFormat(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUTextureType type, SDL_GPUTextureUsageFlags usage)
   * @brief Queries whether a texture format is supported for a given type and usage.
   * @param self The RenderDevice.
   * @param format The texture format to query.
   * @param type The texture dimensionality (2D, cube, 3D, etc.).
   * @param usage Bitmask of intended usages (sampler, colour target, storage, etc.).
   * @return `true` if the format/type/usage combination is supported.
   * @memberof RenderDevice
   */
  bool (*textureSupportsFormat)(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUTextureType type, SDL_GPUTextureUsageFlags usage);

  /**
   * @fn bool RenderDevice::textureSupportsSampleCount(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUSampleCount sample_count)
   * @brief Queries whether a texture format supports a given MSAA sample count.
   * @param self The RenderDevice.
   * @param format The texture format to query.
   * @param sample_count The MSAA sample count to test.
   * @return `true` if the format supports the requested sample count.
   * @memberof RenderDevice
   */
  bool (*textureSupportsSampleCount)(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUSampleCount sample_count);

  /**
   * @fn void RenderDevice::unmapTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf)
   * @brief Unmaps a transfer buffer previously mapped with `mapTransferBuffer`.
   * @details The CPU pointer returned by `mapTransferBuffer` must not be accessed
   *   after this call.
   * @param self The RenderDevice.
   * @param tbuf The transfer buffer to unmap.
   * @memberof RenderDevice
   */
  void (*unmapTransferBuffer)(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf);

  /**
   * @fn bool RenderDevice::waitForFences(const RenderDevice *self, bool wait_all, SDL_GPUFence *const *fences, Uint32 num_fences)
   * @brief Blocks the calling thread until one or all of the given fences are signaled.
   * @param self The RenderDevice.
   * @param wait_all If `true`, wait for all fences; if `false`, return when any one signals.
   * @param fences Array of fences to wait on.
   * @param num_fences Number of fences in @p fences.
   * @return `true` on success, `false` on error.
   * @memberof RenderDevice
   */
  bool (*waitForFences)(const RenderDevice *self, bool wait_all, SDL_GPUFence *const *fences, Uint32 num_fences);

  /**
   * @fn bool RenderDevice::waitForIdle(const RenderDevice *self)
   * @brief Blocks until the GPU has finished all submitted work.
   * @details Use sparingly — typically only at shutdown or before releasing
   *   GPU resources that may still be in use.
   * @param self The RenderDevice.
   * @return `true` on success, `false` on error.
   * @memberof RenderDevice
   */
  bool (*waitForIdle)(const RenderDevice *self);

  /**
   * @fn bool RenderDevice::waitForSwapchain(const RenderDevice *self)
   * @brief Blocks until this device's swapchain is available for the next frame.
   * @param self The RenderDevice.
   * @return `true` on success, `false` on error.
   * @memberof RenderDevice
   */
  bool (*waitForSwapchain)(const RenderDevice *self);
};

/**
 * @fn Class *RenderDevice::_RenderDevice(void)
 * @brief The RenderDevice archetype.
 * @return The RenderDevice Class.
 * @memberof RenderDevice
 */
OBJECTIVELYGPU_EXPORT Class *_RenderDevice(void);
