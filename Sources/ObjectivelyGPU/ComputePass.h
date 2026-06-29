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
 * @brief ComputePass wraps an `SDL_GPUComputePass` for recording compute dispatches.
 */

typedef struct CommandBuffer CommandBuffer;
typedef struct ComputePass ComputePass;
typedef struct ComputePassInterface ComputePassInterface;

/**
 * @brief A scoped compute pass for recording GPU compute dispatches.
 *
 * Obtain a ComputePass from `CommandBuffer::beginComputePass`. When all
 * dispatches have been recorded, release the pass — `dealloc` calls
 * `SDL_EndGPUComputePass` automatically so no explicit `end` is required:
 *
 * @code
 *   ComputePass *compute = $(commands, beginComputePass, NULL, 0, storageBuffers, 1);
 *   $(compute, bindPipeline, pipeline);
 *   $(compute, dispatchCompute, groupsX, groupsY, 1);
 *   release(compute);  // ends the pass
 * @endcode
 *
 * @extends Object
 */
struct ComputePass {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  ComputePassInterface *interface;

  /**
   * @brief The CommandBuffer this pass was begun from.
   * @private
   */
  CommandBuffer *commands;

  /**
   * @brief The underlying SDL compute pass.
   * @details Set to NULL by `dealloc` after `SDL_EndGPUComputePass` is called.
   * @private
   */
  SDL_GPUComputePass *pass;

  /**
   * @brief The currently bound pipeline, or `NULL`.
   */
  SDL_GPUComputePipeline *pipeline;
};

/**
 * @brief The ComputePass interface.
 */
struct ComputePassInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn void ComputePass::bindPipeline(const ComputePass *self, SDL_GPUComputePipeline *pipeline)
   * @brief Binds a compute pipeline for subsequent dispatches.
   * @memberof ComputePass
   */
  void (*bindPipeline)(const ComputePass *self, SDL_GPUComputePipeline *pipeline);

  /**
   * @fn void ComputePass::bindSamplers(const ComputePass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num)
   * @brief Binds texture-sampler pairs for the compute stage.
   * @memberof ComputePass
   */
  void (*bindSamplers)(const ComputePass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num);

  /**
   * @fn void ComputePass::bindStorageBuffers(const ComputePass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num)
   * @brief Binds read-only storage buffers for the compute stage.
   * @memberof ComputePass
   */
  void (*bindStorageBuffers)(const ComputePass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num);

  /**
   * @fn void ComputePass::bindStorageTextures(const ComputePass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num)
   * @brief Binds read-only storage textures for the compute stage.
   * @memberof ComputePass
   */
  void (*bindStorageTextures)(const ComputePass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num);

  /**
   * @fn void ComputePass::dispatchCompute(const ComputePass *self, Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ)
   * @brief Dispatches a compute workload.
   * @param self The ComputePass.
   * @param groupCountX Number of workgroups in the X dimension.
   * @param groupCountY Number of workgroups in the Y dimension.
   * @param groupCountZ Number of workgroups in the Z dimension.
   * @memberof ComputePass
   */
  void (*dispatchCompute)(const ComputePass *self, Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ);

  /**
   * @fn void ComputePass::dispatchComputeIndirect(const ComputePass *self, SDL_GPUBuffer *buffer, Uint32 offset)
   * @brief Dispatches a compute workload with parameters sourced from a GPU buffer.
   * @memberof ComputePass
   */
  void (*dispatchComputeIndirect)(const ComputePass *self, SDL_GPUBuffer *buffer, Uint32 offset);

  /**
   * @fn ComputePass *ComputePass::init(ComputePass *self, CommandBuffer *commands, SDL_GPUComputePass *pass)
   * @brief Initializes this ComputePass wrapping the given SDL compute pass.
   * @param self The ComputePass.
   * @param commands The CommandBuffer that created this pass.
   * @param pass The SDL compute pass to wrap. Must not be NULL.
   * @return The initialized ComputePass, or NULL on failure.
   * @memberof ComputePass
   */
  ComputePass *(*init)(ComputePass *self, CommandBuffer *commands, SDL_GPUComputePass *pass);
};

/**
 * @fn Class *ComputePass::_ComputePass(void)
 * @brief The ComputePass archetype.
 * @return The ComputePass Class.
 * @memberof ComputePass
 */
OBJECTIVELYGPU_EXPORT Class *_ComputePass(void);
