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

#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief CommandBuffer wraps an `SDL_GPUCommandBuffer` for a single frame.
 */

typedef struct ComputePass ComputePass;
typedef struct CopyPass CopyPass;
typedef struct RenderPass RenderPass;
typedef struct CommandBuffer CommandBuffer;
typedef struct CommandBufferInterface CommandBufferInterface;

/**
 * @brief A recorded sequence of GPU commands for a single frame.
 *
 * CommandBuffer is a lightweight wrapper around `SDL_GPUCommandBuffer`.
 * Obtain one from `RenderDevice::acquireCommandBuffer` each frame. The
 * three `begin*Pass` factory methods return the appropriate pass object;
 * releasing the pass ends it, ready for the next pass or submission.
 *
 * Submit or cancel the buffer with `submit`, `submitAndFence`, or `cancel`.
 * Releasing a CommandBuffer without submitting or cancelling it is an error.
 *
 * @extends Object
 */
struct CommandBuffer {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  CommandBufferInterface *interface;

  /**
   * @brief The underlying SDL command buffer.
   */
  SDL_GPUCommandBuffer *cmd;

  /**
   * @brief The GPU device, threaded through to pass objects that need it.
   * @private
   */
  SDL_GPUDevice *device;
};

/**
 * @brief The CommandBuffer interface.
 */
struct CommandBufferInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn bool CommandBuffer::acquireSwapchainTexture(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h)
   * @brief Acquires the next swapchain texture for rendering.
   * @param self The CommandBuffer.
   * @param window The window whose swapchain to acquire.
   * @param texture Receives the swapchain texture, or NULL if unavailable this frame.
   * @param w Receives the swapchain width in pixels, or NULL.
   * @param h Receives the swapchain height in pixels, or NULL.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*acquireSwapchainTexture)(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h);

  /**
   * @fn ComputePass *CommandBuffer::beginComputePass(const CommandBuffer *self, ...)
   * @brief Begins a compute pass and returns a retained ComputePass.
   * @details The returned ComputePass must be released when compute work is
   *   complete. Releasing it calls `SDL_EndGPUComputePass` automatically.
   * @param self The CommandBuffer.
   * @param storageTextures Read-write storage texture bindings, or NULL.
   * @param numStorageTextures Number of storage texture bindings.
   * @param storageBuffers Read-write storage buffer bindings, or NULL.
   * @param numStorageBuffers Number of storage buffer bindings.
   * @return A new ComputePass, retained. Must be released by the caller.
   * @memberof CommandBuffer
   */
  ComputePass *(*beginComputePass)(const CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers);

  /**
   * @fn CopyPass *CommandBuffer::beginCopyPass(const CommandBuffer *self)
   * @brief Begins a copy pass and returns a retained CopyPass.
   * @details The returned CopyPass must be released when all transfers are
   *   complete. Releasing it calls `SDL_EndGPUCopyPass` automatically.
   * @param self The CommandBuffer.
   * @return A new CopyPass, retained. Must be released by the caller.
   * @memberof CommandBuffer
   */
  CopyPass *(*beginCopyPass)(const CommandBuffer *self);

  /**
   * @fn RenderPass *CommandBuffer::beginRenderPass(const CommandBuffer *self, ...)
   * @brief Begins a render pass and returns a retained RenderPass.
   * @details The returned RenderPass must be released when all draw calls are
   *   recorded. Releasing it calls `SDL_EndGPURenderPass` automatically.
   * @param self The CommandBuffer.
   * @param colorTargets Array of color target infos.
   * @param numColorTargets Number of color targets.
   * @param depthStencil Depth-stencil target info, or NULL.
   * @return A new RenderPass, retained. Must be released by the caller.
   * @memberof CommandBuffer
   */
  RenderPass *(*beginRenderPass)(const CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil);

  /**
   * @fn void CommandBuffer::blitTexture(const CommandBuffer *self, const SDL_GPUBlitInfo *info)
   * @brief Blits a region of one texture to another.
   * @memberof CommandBuffer
   */
  void (*blitTexture)(const CommandBuffer *self, const SDL_GPUBlitInfo *info);

  /**
   * @fn bool CommandBuffer::cancel(const CommandBuffer *self)
   * @brief Cancels this command buffer without submitting it to the GPU.
   * @details Use this when swapchain acquisition fails or rendering must be
   *   skipped for a frame. The CommandBuffer must be released after cancel.
   * @param self The CommandBuffer.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*cancel)(const CommandBuffer *self);

  /**
   * @fn void CommandBuffer::generateMipmaps(const CommandBuffer *self, SDL_GPUTexture *texture)
   * @brief Records a mipmap generation command for the given texture.
   * @memberof CommandBuffer
   */
  void (*generateMipmaps)(const CommandBuffer *self, SDL_GPUTexture *texture);

  /**
   * @fn CommandBuffer *CommandBuffer::initWithCommandBuffer(CommandBuffer *self, SDL_GPUCommandBuffer *cmd)
   * @brief Initializes this CommandBuffer wrapping the given SDL command buffer.
   * @param self The CommandBuffer.
   * @param cmd The SDL command buffer to wrap. Must not be NULL.
   * @return The initialized CommandBuffer, or NULL on failure.
   * @memberof CommandBuffer
   */
  CommandBuffer *(*initWithCommandBuffer)(CommandBuffer *self, SDL_GPUCommandBuffer *cmd);

  /**
   * @fn void CommandBuffer::insertDebugLabel(const CommandBuffer *self, const char *text)
   * @brief Inserts an arbitrary debug label into the command stream.
   * @memberof CommandBuffer
   */
  void (*insertDebugLabel)(const CommandBuffer *self, const char *text);

  /**
   * @fn void CommandBuffer::popDebugGroup(const CommandBuffer *self)
   * @brief Ends the innermost active debug group.
   * @memberof CommandBuffer
   */
  void (*popDebugGroup)(const CommandBuffer *self);

  /**
   * @fn void CommandBuffer::pushComputeUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
   * @brief Pushes uniform data for the current compute shader.
   * @memberof CommandBuffer
   */
  void (*pushComputeUniformData)(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length);

  /**
   * @fn void CommandBuffer::pushDebugGroup(const CommandBuffer *self, const char *name)
   * @brief Begins a named debug group in the command stream.
   * @memberof CommandBuffer
   */
  void (*pushDebugGroup)(const CommandBuffer *self, const char *name);

  /**
   * @fn void CommandBuffer::pushFragmentUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
   * @brief Pushes uniform data for the current fragment shader.
   * @memberof CommandBuffer
   */
  void (*pushFragmentUniformData)(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length);

  /**
   * @fn void CommandBuffer::pushVertexUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
   * @brief Pushes uniform data for the current vertex shader.
   * @memberof CommandBuffer
   */
  void (*pushVertexUniformData)(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length);

  /**
   * @fn bool CommandBuffer::submit(const CommandBuffer *self)
   * @brief Submits this command buffer to the GPU for execution.
   * @details All passes must be released before submitting. The CommandBuffer
   *   must be released after submission.
   * @param self The CommandBuffer.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*submit)(const CommandBuffer *self);

  /**
   * @fn SDL_GPUFence *CommandBuffer::submitAndFence(const CommandBuffer *self)
   * @brief Submits this command buffer and returns a fence for GPU completion.
   * @details The fence must be released via `RenderDevice::releaseFence` when
   *   no longer needed.
   * @param self The CommandBuffer.
   * @return A new SDL_GPUFence, or NULL on error.
   * @memberof CommandBuffer
   */
  SDL_GPUFence *(*submitAndFence)(const CommandBuffer *self);

  /**
   * @fn bool CommandBuffer::waitAndAcquireSwapchainTexture(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h)
   * @brief Blocks until a swapchain texture is available, then acquires it.
   * @details Prefer `acquireSwapchainTexture` unless you must block.
   * @param self The CommandBuffer.
   * @param window The window whose swapchain to acquire.
   * @param texture Receives the swapchain texture.
   * @param w Receives the swapchain width in pixels, or NULL.
   * @param h Receives the swapchain height in pixels, or NULL.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*waitAndAcquireSwapchainTexture)(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h);
};

/**
 * @fn Class *CommandBuffer::_CommandBuffer(void)
 * @brief The CommandBuffer archetype.
 * @return The CommandBuffer Class.
 * @memberof CommandBuffer
 */
OBJECTIVELYGPU_EXPORT Class *_CommandBuffer(void);
