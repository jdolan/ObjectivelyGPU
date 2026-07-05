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

#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief CommandBuffer wraps an `SDL_GPUCommandBuffer` for a single frame.
 */

typedef struct ComputePass ComputePass;
typedef struct CopyPass CopyPass;
typedef struct RenderPass RenderPass;
typedef struct RenderDevice RenderDevice;
typedef struct CommandBuffer CommandBuffer;
typedef struct CommandBufferInterface CommandBufferInterface;
typedef struct SwapchainTexture SwapchainTexture;

/**
 * @brief The swapchain texture acquired for a single frame.
 * @details Valid only between a successful `acquireSwapchainTexture` call and
 * the `submit` of the same `CommandBuffer`. SDL3 reclaims the texture on submit.
 */
struct SwapchainTexture {

  /**
   * @brief The swapchain render target for this frame.
   */
  SDL_GPUTexture *texture;

  /**
   * @brief The swapchain texture format.
   */
  SDL_GPUTextureFormat format;

  /**
   * @brief The swapchain dimensions in pixels.
   */
  SDL_Size size;
};

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
  SDL_GPUCommandBuffer *commands;

  /**
   * @brief The RenderDevice that this CommandBuffer belongs to.
   */
  RenderDevice *device;

  /**
   * @brief The currently open pass (RenderPass, CopyPass, or ComputePass), or `NULL`.
   * @details Borrowed reference, set by the `begin*Pass` factories and cleared when the
   *   pass ends. Used to enforce that at most one pass is open at a time.
   * @private
   */
  Object *pass;

  /**
   * @brief Whether this command buffer has been submitted or cancelled.
   * @private
   */
  bool submitted;
  
  /**
   * @brief User data.
   */
  ident data;
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
   * @fn bool CommandBuffer::acquireSwapchainTexture(const CommandBuffer *self, SwapchainTexture *swapchain)
   * @brief Acquires the next swapchain texture for rendering.
   * @details Returns `false` (without asserting) when the window is minimised
   *   or the swapchain is temporarily unavailable. The caller should skip
   *   rendering for that frame.
   * @param self The CommandBuffer.
   * @param swapchain Output structure populated with the texture and dimensions.
   * @return True on success, false when the swapchain is unavailable this frame.
   * @memberof CommandBuffer
   */
  bool (*acquireSwapchainTexture)(const CommandBuffer *self, SwapchainTexture *swapchain);

  /**
   * @fn ComputePass *CommandBuffer::beginComputePass(CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers)
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
  ComputePass *(*beginComputePass)(CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers);

  /**
   * @fn CopyPass *CommandBuffer::beginCopyPass(CommandBuffer *self)
   * @brief Begins a copy pass and returns a retained CopyPass.
   * @details The returned CopyPass must be released when all transfers are
   *   complete. Releasing it calls `SDL_EndGPUCopyPass` automatically.
   * @param self The CommandBuffer.
   * @return A new CopyPass, retained. Must be released by the caller.
   * @memberof CommandBuffer
   */
  CopyPass *(*beginCopyPass)(CommandBuffer *self);

  /**
   * @fn RenderPass *CommandBuffer::beginRenderPass(CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil)
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
  RenderPass *(*beginRenderPass)(CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil);

  /**
   * @fn void CommandBuffer::blitTexture(const CommandBuffer *self, const SDL_GPUBlitInfo *info)
   * @brief Blits a region of one texture to another.
   * @memberof CommandBuffer
   */
  void (*blitTexture)(const CommandBuffer *self, const SDL_GPUBlitInfo *info);

  /**
   * @fn bool CommandBuffer::cancel(CommandBuffer *self)
   * @brief Cancels this command buffer without submitting it to the GPU.
   * @details Use this when swapchain acquisition fails or rendering must be
   *   skipped for a frame. The CommandBuffer must be released after cancel.
   * @param self The CommandBuffer.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*cancel)(CommandBuffer *self);

  /**
   * @fn void CommandBuffer::generateMipmaps(const CommandBuffer *self, SDL_GPUTexture *texture)
   * @brief Records a mipmap generation command for the given texture.
   * @memberof CommandBuffer
   */
  void (*generateMipmaps)(const CommandBuffer *self, SDL_GPUTexture *texture);

  /**
   * @fn CommandBuffer *CommandBuffer::initWithCommandBuffer(CommandBuffer *self, const RenderDevice *device, *SDL_GPUCommandBuffer *commands)
   * @brief Initializes this CommandBuffer wrapping the given SDL command buffer.
   * @param self The CommandBuffer.
   * @param device The RenderDevice that created this CommandBuffer.
   * @param commands The SDL command buffer to wrap. Must not be NULL.
   * @return The initialized CommandBuffer, or NULL on failure.
   * @memberof CommandBuffer
   */
  CommandBuffer *(*initWithCommandBuffer)(CommandBuffer *self, const RenderDevice *device, SDL_GPUCommandBuffer *commands);

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
   * @fn void CommandBuffer::pushUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
   * @brief Pushes the same uniform data to @p slot for both the current vertex
   *   and fragment shaders.
   * @details Convenience for the common case of a uniform block (e.g. globals,
   *   a material block) declared at the same binding in both stages. Equivalent
   *   to calling `pushVertexUniformData` and `pushFragmentUniformData` with the
   *   same arguments.
   * @param self The CommandBuffer.
   * @param slot The uniform buffer slot, matching both shaders' declarations.
   * @param data The uniform data to push.
   * @param length The length of @p data in bytes.
   * @memberof CommandBuffer
   */
  void (*pushUniformData)(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length);

  /**
   * @fn void CommandBuffer::pushVertexUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
   * @brief Pushes uniform data for the current vertex shader.
   * @memberof CommandBuffer
   */
  void (*pushVertexUniformData)(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length);

  /**
   * @fn bool CommandBuffer::submit(CommandBuffer *self)
   * @brief Submits this command buffer to the GPU for execution.
   * @details All passes must be released before submitting. The CommandBuffer
   *   must be released after submission.
   * @param self The CommandBuffer.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*submit)(CommandBuffer *self);

  /**
   * @fn SDL_GPUFence *CommandBuffer::submitAndFence(CommandBuffer *self)
   * @brief Submits this command buffer and returns a fence for GPU completion.
   * @details The fence must be released via `RenderDevice::releaseFence` when
   *   no longer needed.
   * @param self The CommandBuffer.
   * @return A new SDL_GPUFence, or NULL on error.
   * @memberof CommandBuffer
   */
  SDL_GPUFence *(*submitAndFence)(CommandBuffer *self);

  /**
   * @fn bool CommandBuffer::waitAndAcquireSwapchainTexture(const CommandBuffer *self, SwapchainTexture *swapchain)
   * @brief Blocks until a swapchain texture is available, then acquires it.
   * @details Prefer `acquireSwapchainTexture` unless you must guarantee a
   *   texture this frame (e.g. during resize).
   * @param self The CommandBuffer.
   * @param swapchain Output structure populated with the texture and dimensions.
   * @return True on success, false on error.
   * @memberof CommandBuffer
   */
  bool (*waitAndAcquireSwapchainTexture)(const CommandBuffer *self, SwapchainTexture *swapchain);
};

/**
 * @fn Class *CommandBuffer::_CommandBuffer(void)
 * @brief The CommandBuffer archetype.
 * @return The CommandBuffer Class.
 * @memberof CommandBuffer
 */
OBJECTIVELYGPU_EXPORT Class *_CommandBuffer(void);
