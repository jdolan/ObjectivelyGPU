/*
 * ObjectivelyGPU: Object oriented MVC framework for SDL3 and GNU C.
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

#include <Objectively/Object.h>

#include <ObjectivelyGPU/CommandBuffer.h>
#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief RenderDevice owns `SDL_gpu` infrastructure for a window and frame loop.
 */

typedef struct CommandBuffer CommandBuffer;
typedef struct RenderDevice RenderDevice;

/**
 * @brief The Swapchain (render target) type.
 */
struct Swapchain {

  /**
   * @brief The current render target.
   * @private
   */
  SDL_GPUTexture *texture;

  /**
   * @brief The swapchain dimensions.
   * @private
   */
  SDL_Size size;
};

typedef struct Swapchain Swapchain;

typedef struct RenderDeviceInterface RenderDeviceInterface;

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
   * @fn bool RenderDevice::acquireSwapchainTexture(const RenderDevice *self, CommandBuffer *cmd, Swapchain *swapchain)
   * @brief Acquires the swapchain render target for the current frame.
   * @details Wraps `SDL_AcquireGPUSwapchainTexture` using `self->window`. On
   *   success, fills `swapchain->texture` with the current back-buffer and
   *   `swapchain->size` with its pixel dimensions. Returns `false` (without
   *   asserting) when the window is minimised or the swapchain is unavailable.
   * @param self The RenderDevice.
   * @param cmd The active CommandBuffer for this frame.
   * @param swapchain Output structure to populate with the texture and dimensions.
   * @return `true` if a swapchain texture was acquired; `false` otherwise.
   * @memberof RenderDevice
   */
  bool (*acquireSwapchainTexture)(const RenderDevice *self, CommandBuffer *cmd, Swapchain *swapchain);

  /**
   * @fn SDL_GPUBuffer *RenderDevice::createBuffer(const RenderDevice *self, const SDL_GPUBufferCreateInfo *info)
   * @brief Creates a GPU-side buffer (vertex, index, indirect, etc.).
   * @param self The RenderDevice.
   * @param info Buffer creation parameters (usage flags, size).
   * @return A new `SDL_GPUBuffer`. GPU_Asserts on failure. Release with `releaseBuffer`.
   * @memberof RenderDevice
   */
  SDL_GPUBuffer *(*createBuffer)(const RenderDevice *self, const SDL_GPUBufferCreateInfo *info);

  /**
   * @fn SDL_GPUComputePipeline *RenderDevice::createComputePipeline(const RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info)
   * @brief Creates a compute pipeline from a compiled compute shader.
   * @param self The RenderDevice.
   * @param info Compute pipeline creation parameters, including the shader.
   * @return A new `SDL_GPUComputePipeline`. GPU_Asserts on failure. Release with `releaseComputePipeline`.
   * @memberof RenderDevice
   */
  SDL_GPUComputePipeline *(*createComputePipeline)(const RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info);

  /**
   * @fn SDL_GPUGraphicsPipeline *RenderDevice::createGraphicsPipeline(const RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info)
   * @brief Creates a graphics pipeline from vertex and fragment shaders plus render state.
   * @param self The RenderDevice.
   * @param info Graphics pipeline creation parameters (shaders, vertex layout,
   *   blend state, rasteriser state, depth/stencil state, colour target formats).
   * @return A new `SDL_GPUGraphicsPipeline`. GPU_Asserts on failure. Release with `releaseGraphicsPipeline`.
   * @memberof RenderDevice
   */
  SDL_GPUGraphicsPipeline *(*createGraphicsPipeline)(const RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info);

  /**
   * @fn SDL_GPUSampler *RenderDevice::createSampler(const RenderDevice *self, const SDL_GPUSamplerCreateInfo *info)
   * @brief Creates a texture sampler describing filter and address modes.
   * @param self The RenderDevice.
   * @param info Sampler creation parameters (min/mag filter, mip mode, address modes, anisotropy, etc.).
   * @return A new `SDL_GPUSampler`. GPU_Asserts on failure. Release with `releaseSampler`.
   * @memberof RenderDevice
   */
  SDL_GPUSampler *(*createSampler)(const RenderDevice *self, const SDL_GPUSamplerCreateInfo *info);

  /**
   * @fn SDL_GPUShader *RenderDevice::createShader(const RenderDevice *self, const SDL_GPUShaderCreateInfo *info)
   * @brief Creates a GPU shader from a fully-filled `SDL_GPUShaderCreateInfo`.
   * @details All fields of @p info, including `code`, `code_size`, and `format`,
   *   must be set by the caller. Prefer `loadShader` to load compiled blobs from
   *   the resource system with automatic format selection.
   * @param self The RenderDevice.
   * @param info Shader creation parameters with all fields populated.
   * @return A new `SDL_GPUShader`. GPU_Asserts on failure. Release with `releaseShader`.
   * @memberof RenderDevice
   */
  SDL_GPUShader *(*createShader)(const RenderDevice *self, const SDL_GPUShaderCreateInfo *info);

  /**
   * @fn SDL_GPUTexture *RenderDevice::createTexture(const RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels)
   * @brief Creates a GPU texture, optionally uploading initial pixel data.
   * @details When @p pixels is non-NULL, allocates a temporary upload transfer
   *   buffer, copies @p pixels into it, records a copy pass, and submits
   *   immediately so the texture is GPU-resident before returning.
   * @param self The RenderDevice.
   * @param info Texture creation parameters (format, type, dimensions, usage, etc.).
   * @param pixels Initial pixel data to upload, or `NULL` to leave the texture uninitialised.
   * @return A new `SDL_GPUTexture`. GPU_Asserts on failure. Release with `releaseTexture`.
   * @memberof RenderDevice
   */
  SDL_GPUTexture *(*createTexture)(const RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels);

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
   * @fn SDL_GPUTextureFormat RenderDevice::getSwapchainTextureFormat(const RenderDevice *self, SDL_Window *window)
   * @brief Returns the pixel format of the swapchain for the given window.
   * @details Useful for configuring render-pass colour target formats or creating
   *   pipelines that write to the swapchain.
   * @param self The RenderDevice.
   * @param window The window whose swapchain format to query.
   * @return The `SDL_GPUTextureFormat` of the window's swapchain.
   * @memberof RenderDevice
   */
  SDL_GPUTextureFormat (*getSwapchainTextureFormat)(const RenderDevice *self, SDL_Window *window);

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
   * @fn SDL_GPUShader *RenderDevice::loadShader(const RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info)
   * @brief Loads a compiled shader blob via the Resource system and creates a GPU shader.
   * @details Appends the platform-appropriate extension to @c name and resolves it via
   *   Objectively's ResourceProvider chain:
   *   - Metal (macOS/iOS): `.msl`
   *   - Vulkan (Linux/Android): `.spv`
   *   - D3D12 (Windows): `.dxil`
   *   The caller fills in @c stage, @c entrypoint, and binding counts in @c info.
   *   @c code, @c code_size, and @c format are filled in by this method.
   *   Shader blobs are produced offline by @c sdl-shadercross from HLSL source.
   * @param self The RenderDevice.
   * @param name Shader base name without extension, e.g. @c "shaders/Renderer.vert".
   * @param info Shader creation parameters. @c code, @c code_size, and @c format
   *   are ignored; all other fields must be set by the caller.
   * @return A new SDL_GPUShader. GPU_Asserts on failure.
   * @memberof RenderDevice
   */
  SDL_GPUShader *(*loadShader)(const RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info);

  /**
  * @fn SDL_GPUComputePipeline *RenderDevice::loadComputePipeline(const RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
  * @brief Loads a compiled compute shader blob via the Resource system and creates a compute pipeline.
  * @details Parallel to `loadShader` for compute stages. Appends the platform-appropriate
  *   extension to @c name and resolves it via Objectively's ResourceProvider chain:
  *   - Metal (macOS/iOS): `.msl`
  *   - Vulkan (Linux/Android): `.spv`
  *   - D3D12 (Windows): `.dxil`
  *   The caller fills in @c entrypoint, thread counts, and binding counts in @c info.
  *   @c code, @c code_size, and @c format are filled in by this method.
  *   Shader blobs are produced offline by @c sdl-shadercross from HLSL source.
  * @param self The RenderDevice.
  * @param name Shader base name without extension, e.g. @c "HelloCompute.comp".
  * @param info Compute pipeline creation parameters. @c code, @c code_size, and
  *   @c format are ignored; all other fields must be set by the caller.
  * @return A new SDL_GPUComputePipeline. GPU_Asserts on failure.
  * @memberof RenderDevice
  */
  SDL_GPUComputePipeline *(*loadComputePipeline)(const RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info);

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
   * @fn void RenderDevice::releaseBuffer(const RenderDevice *self, SDL_GPUBuffer *buffer)
   * @brief Releases a GPU buffer created by `createBuffer`. Null-safe.
   * @param self The RenderDevice.
   * @param buffer The buffer to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseBuffer)(const RenderDevice *self, SDL_GPUBuffer *buffer);

  /**
   * @fn void RenderDevice::releaseComputePipeline(const RenderDevice *self, SDL_GPUComputePipeline *pipeline)
   * @brief Releases a compute pipeline created by `createComputePipeline`. Null-safe.
   * @param self The RenderDevice.
   * @param pipeline The pipeline to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseComputePipeline)(const RenderDevice *self, SDL_GPUComputePipeline *pipeline);

  /**
   * @fn void RenderDevice::releaseFence(const RenderDevice *self, SDL_GPUFence *fence)
   * @brief Releases a fence returned by `submitAndFence`. Null-safe.
   * @param self The RenderDevice.
   * @param fence The fence to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseFence)(const RenderDevice *self, SDL_GPUFence *fence);

  /**
   * @fn void RenderDevice::releaseGraphicsPipeline(const RenderDevice *self, SDL_GPUGraphicsPipeline *pipeline)
   * @brief Releases a graphics pipeline created by `createGraphicsPipeline`. Null-safe.
   * @param self The RenderDevice.
   * @param pipeline The pipeline to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseGraphicsPipeline)(const RenderDevice *self, SDL_GPUGraphicsPipeline *pipeline);

  /**
   * @fn void RenderDevice::releaseSampler(const RenderDevice *self, SDL_GPUSampler *sampler)
   * @brief Releases a sampler created by `createSampler`. Null-safe.
   * @param self The RenderDevice.
   * @param sampler The sampler to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseSampler)(const RenderDevice *self, SDL_GPUSampler *sampler);

  /**
   * @fn void RenderDevice::releaseShader(const RenderDevice *self, SDL_GPUShader *shader)
   * @brief Releases a shader created by `createShader` or `loadShader`. Null-safe.
   * @param self The RenderDevice.
   * @param shader The shader to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseShader)(const RenderDevice *self, SDL_GPUShader *shader);

  /**
   * @fn void RenderDevice::releaseTexture(const RenderDevice *self, SDL_GPUTexture *texture)
   * @brief Releases a texture created by `createTexture`. Null-safe.
   * @param self The RenderDevice.
   * @param texture The texture to release, or `NULL`.
   * @memberof RenderDevice
   */
  void (*releaseTexture)(const RenderDevice *self, SDL_GPUTexture *texture);

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
   * @fn void RenderDevice::setBufferName(const RenderDevice *self, SDL_GPUBuffer *buffer, const char *name)
   * @brief Assigns a debug label to a GPU buffer, visible in GPU capture tools.
   * @param self The RenderDevice.
   * @param buffer The buffer to label.
   * @param name A null-terminated debug name string.
   * @memberof RenderDevice
   */
  void (*setBufferName)(const RenderDevice *self, SDL_GPUBuffer *buffer, const char *name);

  /**
   * @fn bool RenderDevice::setSwapchainParameters(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode)
   * @brief Configures swapchain composition and present mode for a window.
   * @details Use `windowSupportsSwapchainComposition` and `windowSupportsPresentMode`
   *   to guard against unsupported combinations before calling this.
   * @param self The RenderDevice.
   * @param window The window whose swapchain to configure.
   * @param composition Colour space / HDR composition mode.
   * @param mode Presentation mode (vsync, mailbox, immediate, etc.).
   * @return `true` on success, `false` if the combination is unsupported.
   * @memberof RenderDevice
   */
  bool (*setSwapchainParameters)(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode);

  /**
   * @fn void RenderDevice::setTextureName(const RenderDevice *self, SDL_GPUTexture *texture, const char *name)
   * @brief Assigns a debug label to a GPU texture, visible in GPU capture tools.
   * @param self The RenderDevice.
   * @param texture The texture to label.
   * @param name A null-terminated debug name string.
   * @memberof RenderDevice
   */
  void (*setTextureName)(const RenderDevice *self, SDL_GPUTexture *texture, const char *name);

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
   * @fn void RenderDevice::submit(const RenderDevice *self, CommandBuffer *cmd)
   * @brief Submits a recorded CommandBuffer to the GPU for execution.
   * @details The CommandBuffer's underlying `SDL_GPUCommandBuffer` is consumed;
   *   the caller must still `release` the CommandBuffer object.
   * @param self The RenderDevice.
   * @param cmd The CommandBuffer to submit.
   * @memberof RenderDevice
   */
  void (*submit)(const RenderDevice *self, CommandBuffer *cmd);

  /**
   * @fn SDL_GPUFence *RenderDevice::submitAndFence(const RenderDevice *self, CommandBuffer *cmd)
   * @brief Submits a CommandBuffer and returns a fence for CPU synchronisation.
   * @details The fence becomes signaled when all GPU work in @p cmd has completed.
   *   Use `queryFence` or `waitForFences` to poll or block on it, then release it
   *   with `releaseFence`.
   * @param self The RenderDevice.
   * @param cmd The CommandBuffer to submit.
   * @return A new `SDL_GPUFence`. GPU_Asserts on failure. Release with `releaseFence`.
   * @memberof RenderDevice
   */
  SDL_GPUFence *(*submitAndFence)(const RenderDevice *self, CommandBuffer *cmd);

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
   * @fn bool RenderDevice::waitForSwapchain(const RenderDevice *self, SDL_Window *window)
   * @brief Blocks until the swapchain for @p window is available for the next frame.
   * @param self The RenderDevice.
   * @param window The window whose swapchain to wait for.
   * @return `true` on success, `false` on error.
   * @memberof RenderDevice
   */
  bool (*waitForSwapchain)(const RenderDevice *self, SDL_Window *window);

  /**
   * @fn bool RenderDevice::windowSupportsPresentMode(const RenderDevice *self, SDL_Window *window, SDL_GPUPresentMode mode)
   * @brief Queries whether a window's swapchain supports the given present mode.
   * @param self The RenderDevice.
   * @param window The window to query.
   * @param mode The present mode to test.
   * @return `true` if the present mode is supported for @p window.
   * @memberof RenderDevice
   */
  bool (*windowSupportsPresentMode)(const RenderDevice *self, SDL_Window *window, SDL_GPUPresentMode mode);

  /**
   * @fn bool RenderDevice::windowSupportsSwapchainComposition(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition)
   * @brief Queries whether a window supports the given swapchain composition (colour space / HDR mode).
   * @param self The RenderDevice.
   * @param window The window to query.
   * @param composition The swapchain composition to test.
   * @return `true` if the composition is supported for @p window.
   * @memberof RenderDevice
   */
  bool (*windowSupportsSwapchainComposition)(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition);
};

/**
 * @fn Class *RenderDevice::_RenderDevice(void)
 * @brief The RenderDevice archetype.
 * @return The RenderDevice Class.
 * @memberof RenderDevice
 */
OBJECTIVELYGPU_EXPORT Class *_RenderDevice(void);
