/*
 * ObjectivelyGPU: Object oriented graphics framework for SDL3 and GNU C.
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

#include <SDL3/SDL_gpu.h>

#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief Framebuffer groups a color and/or depth texture as an off-screen render target.
 */

typedef struct RenderDevice RenderDevice;
typedef struct Framebuffer Framebuffer;
typedef struct FramebufferInterface FramebufferInterface;
typedef struct Texture Texture;

/**
 * @brief Parameters for creating a Framebuffer.
 * @details The GPU-layer analogue of SDL's `*CreateInfo` structs, for a target that
 *   aggregates several `SDL_GPUTexture` attachments and so has no single SDL struct.
 *   Use a designated initializer; omitted fields default to zero
 *   (`SDL_GPU_TEXTUREFORMAT_INVALID`, `SDL_GPU_SAMPLECOUNT_1`).
 */
typedef struct GPU_FramebufferCreateInfo {

  /**
   * @brief Initial framebuffer dimensions.
   * @details When the framebuffer is driven by `RenderDevice::beginFrame`, it is resized
   *   to the swapchain each frame, so this is only the initial allocation size.
   */
  SDL_Size size;

  /**
   * @brief Color attachment format, or `SDL_GPU_TEXTUREFORMAT_INVALID` to omit.
   */
  SDL_GPUTextureFormat colorFormat;

  /**
   * @brief Depth attachment format, or `SDL_GPU_TEXTUREFORMAT_INVALID` to omit.
   */
  SDL_GPUTextureFormat depthFormat;

  /**
   * @brief MSAA sample count; `SDL_GPU_SAMPLECOUNT_1` for no multisampling.
   * @details When greater, a single-sample resolve target is allocated alongside the
   *   multisampled color attachment.
   */
  SDL_GPUSampleCount sampleCount;

} GPU_FramebufferCreateInfo;

/**
 * @brief An off-screen render target grouping a color and/or depth texture.
 *
 * Framebuffer owns a pair of `SDL_GPUTexture` objects (color and/or depth) and
 * provides helpers that produce the `SDL_GPUColorTargetInfo` and
 * `SDL_GPUDepthStencilTargetInfo` values required by `CommandBuffer::beginRenderPass`.
 *
 * Use `SDL_GPU_TEXTUREFORMAT_INVALID` for either format to omit that attachment.
 * Call `resize` whenever the window dimensions change; it releases the existing
 * textures and recreates them at the new size.
 *
 * @code
 *   Framebuffer *fb = $(renderDevice, createFramebuffer, &(GPU_FramebufferCreateInfo) {
 *     .size = { 1920, 1080 },
 *     .colorFormat = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
 *     .depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
 *     .sampleCount = SDL_GPU_SAMPLECOUNT_1,
 *   });
 *
 *   SDL_GPUColorTargetInfo color = $(fb, colorTargetInfo,
 *     SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE,
 *     &(SDL_FColor) { 0, 0, 0, 1 });
 *
 *   SDL_GPUDepthStencilTargetInfo depth = $(fb, depthTargetInfo,
 *     SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);
 *
 *   RenderPass *pass = $(commands, beginRenderPass, &color, 1, &depth);
 *   // ...
 *   release(pass);
 *   release(fb);
 * @endcode
 *
 * @extends Object
 */
struct Framebuffer {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  FramebufferInterface *interface;

  /**
   * @brief The framebuffer dimensions.
   */
  SDL_Size size;

  /**
   * @brief The color attachment texture format, or `SDL_GPU_TEXTUREFORMAT_INVALID` if none.
   */
  SDL_GPUTextureFormat colorFormat;

  /**
   * @brief The depth attachment texture format, or `SDL_GPU_TEXTUREFORMAT_INVALID` if none.
   */
  SDL_GPUTextureFormat depthFormat;

  /**
   * @brief The MSAA sample count of the color and depth attachments.
   * @details `SDL_GPU_SAMPLECOUNT_1` for no multisampling. When greater, `colorTexture`
   *   is multisampled and `resolveTexture` holds the single-sample resolve.
   */
  SDL_GPUSampleCount sampleCount;

  /**
   * @brief The color attachment texture, or `NULL` if `colorFormat` is invalid.
   * @details Multisampled when `sampleCount` is greater than `SDL_GPU_SAMPLECOUNT_1`;
   *   in that case sample the single-sample `resolveTexture`, not this texture.
   */
  Texture *colorTexture;

  /**
   * @brief The single-sample resolve target, or `NULL` unless `sampleCount` is greater than `SDL_GPU_SAMPLECOUNT_1`.
   * @details Render passes resolve `colorTexture` into this texture; it is the texture
   *   to sample, blit, or present. See `resolvedColorTexture`.
   */
  Texture *resolveTexture;

  /**
   * @brief The depth attachment texture, or `NULL` if `depthFormat` is invalid.
   * @private
   */
  Texture *depthTexture;

  /**
   * @brief The owning RenderDevice, used for texture allocation and dealloc.
   * @private
   */
  RenderDevice *device;
};

/**
 * @brief The Framebuffer interface.
 */
struct FramebufferInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn SDL_GPUColorTargetInfo Framebuffer::colorTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor)
   * @brief Returns a populated `SDL_GPUColorTargetInfo` for this framebuffer's color attachment.
   * @details Pass the result directly to `CommandBuffer::beginRenderPass`.
   *   `assert`s that `colorTexture` is non-NULL.
   * @param self The Framebuffer.
   * @param loadOp Load operation at the start of the pass.
   * @param storeOp Store operation at the end of the pass.
   * @param clearColor Clear color used when `loadOp` is `SDL_GPU_LOADOP_CLEAR`, or NULL for black.
   * @return A stack-allocated `SDL_GPUColorTargetInfo`.
   * @memberof Framebuffer
   */
  SDL_GPUColorTargetInfo (*colorTargetInfo)(const Framebuffer *self,
    SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor);

  /**
   * @fn SDL_GPUDepthStencilTargetInfo Framebuffer::depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, float clearDepth)
   * @brief Returns a populated `SDL_GPUDepthStencilTargetInfo` for this framebuffer's depth attachment.
   * @details Pass the result directly to `CommandBuffer::beginRenderPass`.
   *   `assert`s that `depthTexture` is non-NULL.
   * @param self The Framebuffer.
   * @param loadOp Load operation at the start of the pass.
   * @param storeOp Store operation at the end of the pass.
   * @param clearDepth Clear depth value used when `loadOp` is `SDL_GPU_LOADOP_CLEAR`.
   * @return A stack-allocated `SDL_GPUDepthStencilTargetInfo`.
   * @memberof Framebuffer
   */
  SDL_GPUDepthStencilTargetInfo (*depthTargetInfo)(const Framebuffer *self,
    SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, float clearDepth);

  /**
   * @fn Framebuffer *Framebuffer::initWithDevice(Framebuffer *self, RenderDevice *device, const GPU_FramebufferCreateInfo *info)
   * @brief Initializes this Framebuffer and allocates its GPU textures.
   * @param self The Framebuffer.
   * @param device The RenderDevice used to allocate and release textures.
   * @param info Framebuffer creation parameters (size, formats, sample count).
   * @return The initialized Framebuffer, or NULL on failure.
   * @memberof Framebuffer
   */
  Framebuffer *(*initWithDevice)(Framebuffer *self, RenderDevice *device, const GPU_FramebufferCreateInfo *info);

  /**
   * @fn Texture *Framebuffer::resolvedColorTexture(const Framebuffer *self)
   * @brief Returns the single-sample color texture to sample, blit, or present.
   * @details Returns `resolveTexture` when multisampled, otherwise `colorTexture`.
   * @param self The Framebuffer.
   * @return The resolved color texture, or `NULL` if this framebuffer has no color attachment.
   * @memberof Framebuffer
   */
  Texture *(*resolvedColorTexture)(const Framebuffer *self);

  /**
   * @fn bool Framebuffer::resize(Framebuffer *self, const SDL_Size *size)
   * @brief Releases the existing GPU textures and recreates them at @p size.
   * @details Call this when the window is resized. Returns `false` if @p size
   *   matches the current size and no reallocation is needed.
   * @param self The Framebuffer.
   * @param size The new framebuffer dimensions.
   * @return `true` if textures were reallocated; `false` if the size was unchanged.
   * @memberof Framebuffer
   */
  bool (*resize)(Framebuffer *self, const SDL_Size *size);
};

/**
 * @fn Class *Framebuffer::_Framebuffer(void)
 * @brief The Framebuffer archetype.
 * @return The Framebuffer Class.
 * @memberof Framebuffer
 */
OBJECTIVELYGPU_EXPORT Class *_Framebuffer(void);
