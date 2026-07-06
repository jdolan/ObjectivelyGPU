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
 * @brief RenderPass wraps an `SDL_GPURenderPass` for recording draw commands.
 */

typedef struct CommandBuffer CommandBuffer;
typedef struct GraphicsPipeline GraphicsPipeline;
typedef struct QueryPool QueryPool;
typedef struct RenderPass RenderPass;
typedef struct RenderPassInterface RenderPassInterface;

/**
 * @brief A scoped render pass for recording draw commands into a CommandBuffer.
 *
 * Obtain a RenderPass from `CommandBuffer::beginRenderPass`. When all draw
 * commands have been recorded, release the pass — `dealloc` calls
 * `SDL_EndGPURenderPass` automatically so no explicit `end` is required:
 *
 * @code
 *   RenderPass *pass = $(commands, beginRenderPass, colorTargets, numTargets, NULL);
 *   $(pass, bindPipeline, pipeline);
 *   $(pass, drawPrimitives, vertexCount, 1, 0, 0);
 *   release(pass);  // ends the pass
 * @endcode
 *
 * @extends Object
 */
struct RenderPass {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  RenderPassInterface *interface;

  /**
   * @brief The current blend constants.
   */
  SDL_FColor blendConstants;

  /**
   * @brief The CommandBuffer this pass was begun from.
   * @private
   */
  CommandBuffer *commands;

  /**
   * @brief The underlying SDL render pass.
   * @details Set to NULL by `dealloc` after `SDL_EndGPURenderPass` is called.
   * @private
   */
  SDL_GPURenderPass *pass;

  /**
   * @brief The currently bound pipeline, or `NULL`.
   */
  GraphicsPipeline *pipeline;

  /**
   * @brief The current scissor.
   */
  SDL_Rect scissor;

  /**
   * @brief The current stencil reference.
   */
  Uint8 stencilReference;

  /**
   * @brief The current viewport.
   */
  SDL_GPUViewport viewport;
  
  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The RenderPass interface.
 */
struct RenderPassInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn void RenderPass::bindFragmentSamplers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num)
   * @brief Binds texture-sampler pairs for the fragment stage.
   * @memberof RenderPass
   */
  void (*bindFragmentSamplers)(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num);

  /**
   * @fn void RenderPass::bindFragmentStorageBuffers(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num)
   * @brief Binds read-only storage buffers for the fragment stage.
   * @memberof RenderPass
   */
  void (*bindFragmentStorageBuffers)(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num);

  /**
   * @fn void RenderPass::bindFragmentStorageTextures(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num)
   * @brief Binds read-only storage textures for the fragment stage.
   * @memberof RenderPass
   */
  void (*bindFragmentStorageTextures)(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num);

  /**
   * @fn void RenderPass::bindPipeline(RenderPass *self, GraphicsPipeline *pipeline)
   * @brief Binds a graphics pipeline for subsequent draw calls.
   * @memberof RenderPass
   */
  void (*bindPipeline)(RenderPass *self, GraphicsPipeline *pipeline);

  /**
   * @fn void RenderPass::bindIndexBuffer(const RenderPass *self, const SDL_GPUBufferBinding *binding, SDL_GPUIndexElementSize indexElementSize)
   * @brief Binds an index buffer for indexed draw calls.
   * @memberof RenderPass
   */
  void (*bindIndexBuffer)(const RenderPass *self, const SDL_GPUBufferBinding *binding, SDL_GPUIndexElementSize indexElementSize);

  /**
   * @fn void RenderPass::bindVertexBuffers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUBufferBinding *bindings, Uint32 num)
   * @brief Binds vertex buffers for subsequent draw calls.
   * @memberof RenderPass
   */
  void (*bindVertexBuffers)(const RenderPass *self, Uint32 firstSlot, const SDL_GPUBufferBinding *bindings, Uint32 num);

  /**
   * @fn void RenderPass::bindVertexSamplers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num)
   * @brief Binds texture-sampler pairs for the vertex stage.
   * @memberof RenderPass
   */
  void (*bindVertexSamplers)(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num);

  /**
   * @fn void RenderPass::bindVertexStorageBuffers(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num)
   * @brief Binds read-only storage buffers for the vertex stage.
   * @memberof RenderPass
   */
  void (*bindVertexStorageBuffers)(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num);

  /**
   * @fn void RenderPass::bindVertexStorageTextures(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num)
   * @brief Binds read-only storage textures for the vertex stage.
   * @memberof RenderPass
   */
  void (*bindVertexStorageTextures)(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num);

  /**
   * @fn void RenderPass::beginQuery(const RenderPass *self, QueryPool *pool, Uint32 index)
   * @brief Begins a query, accumulating results from subsequent draw calls until `endQuery`.
   * @details A no-op when @p pool is backed by an unsupported SDL3 build; see `QueryPool`.
   * @memberof RenderPass
   */
  void (*beginQuery)(const RenderPass *self, QueryPool *pool, Uint32 index);

  /**
   * @fn void RenderPass::endQuery(const RenderPass *self, QueryPool *pool, Uint32 index)
   * @brief Ends a query begun with `beginQuery`.
   * @details A no-op when @p pool is backed by an unsupported SDL3 build; see `QueryPool`.
   * @memberof RenderPass
   */
  void (*endQuery)(const RenderPass *self, QueryPool *pool, Uint32 index);

  /**
   * @fn void RenderPass::drawIndexedPrimitives(const RenderPass *self, Uint32 numIndices, Uint32 numInstances, Uint32 firstIndex, Sint32 vertexOffset, Uint32 firstInstance)
   * @brief Draws indexed primitives.
   * @memberof RenderPass
   */
  void (*drawIndexedPrimitives)(const RenderPass *self, Uint32 numIndices, Uint32 numInstances, Uint32 firstIndex, Sint32 vertexOffset, Uint32 firstInstance);

  /**
   * @fn void RenderPass::drawIndexedPrimitivesIndirect(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount)
   * @brief Draws indexed primitives with parameters sourced from a GPU buffer.
   * @memberof RenderPass
   */
  void (*drawIndexedPrimitivesIndirect)(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount);

  /**
   * @fn void RenderPass::drawPrimitives(const RenderPass *self, Uint32 numVertices, Uint32 numInstances, Uint32 firstVertex, Uint32 firstInstance)
   * @brief Draws non-indexed primitives.
   * @memberof RenderPass
   */
  void (*drawPrimitives)(const RenderPass *self, Uint32 numVertices, Uint32 numInstances, Uint32 firstVertex, Uint32 firstInstance);

  /**
   * @fn void RenderPass::drawPrimitivesIndirect(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount)
   * @brief Draws non-indexed primitives with parameters sourced from a GPU buffer.
   * @memberof RenderPass
   */
  void (*drawPrimitivesIndirect)(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount);

  /**
   * @fn RenderPass *RenderPass::init(RenderPass *self, CommandBuffer *commands, SDL_GPURenderPass *pass)
   * @brief Initializes this RenderPass wrapping the given SDL render pass.
   * @param self The RenderPass.
   * @param commands The CommandBuffer that created this pass.
   * @param pass The SDL render pass to wrap. Must not be NULL.
   * @return The initialized RenderPass, or NULL on failure.
   * @memberof RenderPass
   */
  RenderPass *(*init)(RenderPass *self, CommandBuffer *commands, SDL_GPURenderPass *pass);

  /**
   * @fn void RenderPass::setBlendConstants(RenderPass *self, SDL_FColor blendConstants)
   * @brief Sets the blend constants used by `SDL_GPU_BLENDFACTOR_CONSTANT_*` factors.
   * @memberof RenderPass
   */
  void (*setBlendConstants)(RenderPass *self, SDL_FColor blendConstants);

  /**
   * @fn void RenderPass::setScissor(RenderPass *self, const SDL_Rect *scissor)
   * @brief Sets the scissor rectangle. Pass NULL to reset to the full viewport.
   * @memberof RenderPass
   */
  void (*setScissor)(RenderPass *self, const SDL_Rect *scissor);

  /**
   * @fn void RenderPass::setStencilReference(RenderPass *self, Uint8 reference)
   * @brief Sets the stencil reference value used in stencil tests.
   * @memberof RenderPass
   */
  void (*setStencilReference)(RenderPass *self, Uint8 reference);

  /**
   * @fn void RenderPass::setViewport(RenderPass *self, const SDL_GPUViewport *viewport)
   * @brief Sets the viewport. Pass NULL to reset to the full render target.
   * @memberof RenderPass
   */
  void (*setViewport)(RenderPass *self, const SDL_GPUViewport *viewport);
};

/**
 * @fn Class *RenderPass::_RenderPass(void)
 * @brief The RenderPass archetype.
 * @return The RenderPass Class.
 * @memberof RenderPass
 */
OBJECTIVELYGPU_EXPORT Class *_RenderPass(void);
