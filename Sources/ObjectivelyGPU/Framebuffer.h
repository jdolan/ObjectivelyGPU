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
 *   Framebuffer *fb = $(alloc(Framebuffer), initWithDevice, renderDevice,
 *     &(SDL_Size) { 1920, 1080 },
 *     SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
 *     SDL_GPU_TEXTUREFORMAT_D32_FLOAT);
 *
 *   SDL_GPUColorTargetInfo color = $(fb, colorTargetInfo,
 *     SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE,
 *     &(SDL_FColor) { 0, 0, 0, 1 });
 *
 *   SDL_GPUDepthStencilTargetInfo depth = $(fb, depthTargetInfo,
 *     SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_DONT_CARE, 1.f);
 *
 *   RenderPass *pass = $(cmd, beginRenderPass, &color, 1, &depth);
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
   * @brief The color attachment texture.
   * @private
   */
  SDL_GPUTexture *colorTexture;

  /**
   * @brief The depth attachment texture.
   * @private
   */
  SDL_GPUTexture *depthTexture;

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
   * @fn Framebuffer *Framebuffer::initWithDevice(Framebuffer *self, RenderDevice *device, const SDL_Size *size, SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat)
   * @brief Initializes this Framebuffer and allocates its GPU textures.
   * @param self The Framebuffer.
   * @param device The RenderDevice used to allocate and release textures.
   * @param size Initial framebuffer dimensions.
   * @param colorFormat Color attachment format, or `SDL_GPU_TEXTUREFORMAT_INVALID` to omit.
   * @param depthFormat Depth attachment format, or `SDL_GPU_TEXTUREFORMAT_INVALID` to omit.
   * @return The initialized Framebuffer, or NULL on failure.
   * @memberof Framebuffer
   */
  Framebuffer *(*initWithDevice)(Framebuffer *self, RenderDevice *device,
    const SDL_Size *size,
    SDL_GPUTextureFormat colorFormat,
    SDL_GPUTextureFormat depthFormat);

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
