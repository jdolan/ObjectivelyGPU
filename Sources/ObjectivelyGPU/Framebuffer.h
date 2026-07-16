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
 * @brief The maximum number of color attachments a Framebuffer may have.
 * @details A render pass binds N color targets plus at most one depth-stencil target;
 *   this caps N. Four matches the minimum guaranteed across backends.
 */
#define GPU_MAX_COLOR_TARGETS 4

/**
 * @brief Describes, and backs, a single Framebuffer attachment (color or depth).
 * @details `format`, the clear value, and `doubleBuffered` are the input description:
 *   set them in `GPU_FramebufferCreateInfo` with a designated initializer. `textures`
 *   and `resolveTextures` are live state, populated by `Framebuffer::resize` and owned
 *   by the Framebuffer -- leave them zeroed in a `GPU_FramebufferCreateInfo`.
 *
 *   When `doubleBuffered` is `true`, the attachment allocates both texture slots and
 *   `Framebuffer::swap` alternates which slot is the current frame's write target, so
 *   the other slot remains valid to sample as last frame's contents -- e.g. a depth
 *   copy sampled by soft particles -- without a same-frame write-then-read hazard.
 *   Single-buffered attachments (`doubleBuffered` `false`) only ever use slot `0`.
 */
typedef struct GPU_FramebufferAttachment {

  /**
   * @brief The attachment format, or `SDL_GPU_TEXTUREFORMAT_INVALID` to omit (depth only).
   */
  SDL_GPUTextureFormat format;

  union {
    /**
     * @brief The clear color, for color attachments.
     * @details Used by `Framebuffer::colorTargetInfo` and `CommandBuffer::beginRenderPassWithFramebuffer`.
     */
    SDL_FColor clearColor;

    /**
     * @brief The clear depth value, for the depth attachment.
     * @details Used by `CommandBuffer::beginRenderPassWithFramebuffer`.
     */
    float clearDepth;
  };

  /**
   * @brief If `true`, this attachment is double-buffered; see above.
   */
  bool doubleBuffered;

  /**
   * @brief The backing texture(s). Multisampled when the Framebuffer's `sampleCount` is
   *   greater than `SDL_GPU_SAMPLECOUNT_1`; sample the corresponding `resolveTextures`
   *   entry instead. Index `1` is only allocated when `doubleBuffered` is `true`.
   * @private
   */
  Texture *textures[2];

  /**
   * @brief The single-sample resolve target(s), or `NULL` unless the Framebuffer is
   *   multisampled. Mirrors `textures`.
   * @private
   */
  Texture *resolveTextures[2];

} GPU_FramebufferAttachment;

/**
 * @brief Parameters for creating a Framebuffer.
 */
typedef struct GPU_FramebufferCreateInfo {

  /**
   * @brief Initial framebuffer dimensions.
   * @details When the framebuffer is driven by `RenderDevice::beginFrame`, it is resized
   *   to the swapchain each frame, so this is only the initial allocation size.
   */
  SDL_Size size;

  /**
   * @brief The color attachments, one per render target (MRT).
   * @details Indices `[0, numColorTargets)` are used. Only `format`, `clearColor`, and
   *   `doubleBuffered` are meaningful here; leave `textures`/`resolveTextures` zeroed.
   */
  GPU_FramebufferAttachment colorAttachments[GPU_MAX_COLOR_TARGETS];

  /**
   * @brief The number of color attachments; `0` to omit color entirely.
   */
  Uint32 numColorTargets;

  /**
   * @brief The depth attachment, or `format` `SDL_GPU_TEXTUREFORMAT_INVALID` to omit.
   * @details A render pass supports at most one depth-stencil target, so this is singular.
   *   Only `format` and `clearDepth` are meaningful here.
   */
  GPU_FramebufferAttachment depthAttachment;

  /**
   * @brief MSAA sample count; `SDL_GPU_SAMPLECOUNT_1` for no multisampling.
   * @details When greater, a single-sample resolve target is allocated alongside each
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
 *     .colorAttachments = {
 *       { .format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT, .clearColor = { 0.f, 0.f, 0.f, 1.f } },
 *     },
 *     .numColorTargets = 1,
 *     .depthAttachment = { .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT, .clearDepth = 1.f },
 *     .sampleCount = SDL_GPU_SAMPLECOUNT_1,
 *   });
 *
 *   // Renders into all of fb's targets, clearing to their own clear color/depth.
 *   RenderPass *pass = $(commands, beginRenderPassWithFramebuffer, fb,
 *     SDL_GPU_LOADOP_CLEAR, SDL_GPU_STOREOP_STORE);
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
   * @brief The owning RenderDevice, used for texture allocation and dealloc.
   */
  RenderDevice *device;

  /**
   * @brief The framebuffer dimensions.
   */
  SDL_Size size;

  /**
   * @brief The MSAA sample count of all color and depth attachments.
   */
  SDL_GPUSampleCount sampleCount;

  /**
   * @brief The color attachments, indices `[0, numColorTargets)`.
   */
  GPU_FramebufferAttachment colorAttachments[GPU_MAX_COLOR_TARGETS];

  /**
   * @brief The number of color attachments; indices `[0, numColorTargets)` are valid.
   */
  Uint32 numColorAttachments;

  /**
   * @brief The depth attachment, or `format` `SDL_GPU_TEXTUREFORMAT_INVALID` if none.
   */
  GPU_FramebufferAttachment depthAttachment;

  /**
   * @brief The current frame parity for double-buffered color attachments; toggled by `swap`.
   * @private
   */
  Uint32 frame;
  
  /**
   * @brief User data.
   */
  ident data;
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
   * @fn SDL_GPUColorTargetInfo Framebuffer::colorTargetInfo(const Framebuffer *self, Uint32 index, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp)
   * @brief Returns a populated `SDL_GPUColorTargetInfo` for color attachment @p index.
   * @details Assemble an array of these (one per color target) and pass it to
   *   `CommandBuffer::beginRenderPass`. When multisampled, the resolve target and a
   *   `RESOLVE_AND_STORE` store op are wired in automatically. `assert`s @p index is valid.
   * @param self The Framebuffer.
   * @param index The color attachment index, in `[0, numColorTargets)`.
   * @param loadOp Load operation at the start of the pass.
   * @param storeOp Store operation at the end of the pass.
   * @return A stack-allocated `SDL_GPUColorTargetInfo`, cleared to the attachment's own
   *   `clearColor` when `loadOp` is `SDL_GPU_LOADOP_CLEAR`.
   * @memberof Framebuffer
   */
  SDL_GPUColorTargetInfo (*colorTargetInfo)(const Framebuffer *self, Uint32 index, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp);

  /**
   * @fn SDL_GPUDepthStencilTargetInfo Framebuffer::depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp)
   * @brief Returns a populated `SDL_GPUDepthStencilTargetInfo` for this framebuffer's depth attachment.
   * @details Pass the result directly to `CommandBuffer::beginRenderPass`.
   *   `assert`s that the depth attachment's texture is non-NULL.
   * @param self The Framebuffer.
   * @param loadOp Load operation at the start of the pass.
   * @param storeOp Store operation at the end of the pass.
   * @return A stack-allocated `SDL_GPUDepthStencilTargetInfo`, cleared to the attachment's
   *   own `clearDepth` when `loadOp` is `SDL_GPU_LOADOP_CLEAR`.
   * @memberof Framebuffer
   */
  SDL_GPUDepthStencilTargetInfo (*depthTargetInfo)(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp);

  /**
   * @fn void Framebuffer::pipelineTargetInfo(const Framebuffer *self, const SDL_GPUColorTargetBlendState *blendStates, SDL_GPUColorTargetDescription *descriptions, SDL_GPUGraphicsPipelineTargetInfo *targetInfo)
   * @brief Populates @p descriptions and @p targetInfo for a GraphicsPipeline that targets this Framebuffer.
   * @details Pass one blend state per color attachment in @p blendStates (see `GraphicsPipelinePresets`), and
   *   caller-owned storage for @p descriptions with capacity `numColorAttachments`. The resulting @p targetInfo
   *   references @p descriptions, so @p descriptions must remain valid for as long as @p targetInfo (and any
   *   `SDL_GPUGraphicsPipelineCreateInfo` built from it) is in use.
   * @param self The Framebuffer.
   * @param blendStates One blend state per color attachment, `[0, numColorAttachments)`.
   * @param descriptions Output storage for `numColorAttachments` color target descriptions.
   * @param targetInfo Output: populated with @p descriptions and this Framebuffer's depth format.
   * @memberof Framebuffer
   */
  void (*pipelineTargetInfo)(const Framebuffer *self, const SDL_GPUColorTargetBlendState *blendStates,
                             SDL_GPUColorTargetDescription *descriptions, SDL_GPUGraphicsPipelineTargetInfo *targetInfo);

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
   * @fn Texture *Framebuffer::resolveColorTexture(const Framebuffer *self, Uint32 index)
   * @brief Returns the single-sample, sampleable color texture for attachment @p index, to sample, blit, or present.
   * @details Returns the current frame's resolved texture: the multisampled resolve target when the
   *   Framebuffer is multisampled, otherwise the attachment's own texture. For a double-buffered
   *   attachment, this is the slot being written *this* frame; see `previousColorTexture` for the other.
   * @param self The Framebuffer.
   * @param index The color attachment index, in `[0, numColorTargets)`.
   * @return The resolved color texture, or `NULL` if @p index has no attachment.
   * @memberof Framebuffer
   */
  Texture *(*resolveColorTexture)(const Framebuffer *self, Uint32 index);

  /**
   * @fn Texture *Framebuffer::previousColorTexture(const Framebuffer *self, Uint32 index)
   * @brief Returns the single-sample, sampleable color texture attachment @p index held *last* frame.
   * @details Only valid for a double-buffered attachment (`doubleBuffered` `true`); `assert`s otherwise.
   *   Since this slot was not written this frame, it may be safely sampled within the same render pass
   *   that is concurrently writing `resolveColorTexture`'s slot -- e.g. soft particles sampling last
   *   frame's depth copy while the current frame's opaque geometry writes this frame's copy, all within
   *   one render pass, with no same-frame write-then-read hazard.
   * @param self The Framebuffer.
   * @param index The color attachment index, in `[0, numColorTargets)`.
   * @return The previous frame's resolved color texture.
   * @memberof Framebuffer
   */
  Texture *(*previousColorTexture)(const Framebuffer *self, Uint32 index);

  /**
   * @fn Texture *Framebuffer::resolveDepthTexture(const Framebuffer *self)
   * @brief Returns the single-sample, sampleable depth texture, to read scene depth (e.g. soft particles).
   * @details Single-sample: the depth attachment itself (created with `SAMPLER`). Multisampled: the
   *   separate resolve texture, which must first be populated by a resolve pass (SDL has no depth
   *   store-op resolve). Returns `NULL` if the framebuffer has no depth attachment.
   * @param self The Framebuffer.
   * @return The sampleable depth texture, or `NULL` if there is no depth attachment.
   * @memberof Framebuffer
   */
  Texture *(*resolveDepthTexture)(const Framebuffer *self);

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

  /**
   * @fn void Framebuffer::swap(Framebuffer *self)
   * @brief Advances the frame parity used by double-buffered attachments.
   * @details Call this once per frame, after any double-buffered attachment has been
   *   written for the frame -- e.g. at the end of the frame. Toggles which physical
   *   slot `colorTargetInfo`/`resolveColorTexture` write/read as "current" for each
   *   `doubleBuffered` attachment; the other slot becomes `previousColorTexture`.
   * @param self The Framebuffer.
   * @memberof Framebuffer
   */
  void (*swap)(Framebuffer *self);
};

/**
 * @fn Class *Framebuffer::_Framebuffer(void)
 * @brief The Framebuffer archetype.
 * @return The Framebuffer Class.
 * @memberof Framebuffer
 */
OBJECTIVELYGPU_EXPORT Class *_Framebuffer(void);
