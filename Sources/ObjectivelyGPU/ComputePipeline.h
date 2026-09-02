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
 * @brief ComputePipeline wraps an `SDL_GPUComputePipeline`, owning its handle.
 */

typedef struct RenderDevice RenderDevice;
typedef struct ComputePipeline ComputePipeline;
typedef struct ComputePipelineInterface ComputePipelineInterface;

/**
 * @brief An `SDL_GPUComputePipeline`: a compiled compute program and its binding layout.
 *
 * ComputePipeline owns its underlying `SDL_GPUComputePipeline` and releases it in
 * `dealloc`, so a pipeline is freed with `release` like any other Objectively object.
 * Bind it with `ComputePass::bindPipeline` via its `pipeline` handle.
 *
 * @extends Object
 */
struct ComputePipeline {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface type.
   * @protected
   */
  ComputePipelineInterface *interface[0];

  /**
   * @brief The underlying SDL compute pipeline.
   */
  SDL_GPUComputePipeline *pipeline;

  /**
   * @brief The RenderDevice that owns this pipeline.
   * @remarks Weak: a resource MUST NOT outlive the device that owns it.
   * @private
   */
  RenderDevice *device;
  
  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The ComputePipeline interface.
 */
struct ComputePipelineInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn ComputePipeline *ComputePipeline::initWithDevice(ComputePipeline *self, RenderDevice *device, const SDL_GPUComputePipelineCreateInfo *info)
   * @brief Initializes this ComputePipeline from a fully-populated `SDL_GPUComputePipelineCreateInfo`.
   * @details The designated (and only) initializer: a thin wrapper over
   *   `SDL_CreateGPUComputePipeline`. All fields of @p info, including `code`,
   *   `code_size`, `format`, and `entrypoint`, must be set by the caller. Prefer
   *   `RenderDevice::loadComputePipeline`, which resolves a compiled blob from the
   *   Resource system, selects the device-appropriate format, and fills these in.
   * @param self The ComputePipeline.
   * @param device The RenderDevice used to create and release the pipeline. Retained.
   * @param info The ComputePipeline creation parameters with all fields populated.
   * @return The initialized ComputePipeline, or `NULL` on failure.
   * @memberof ComputePipeline
   */
  ComputePipeline *(*initWithDevice)(ComputePipeline *self, RenderDevice *device, const SDL_GPUComputePipelineCreateInfo *info);
};

/**
 * @fn Class *ComputePipeline::_ComputePipeline(void)
 * @brief The ComputePipeline archetype.
 * @return The ComputePipeline Class.
 * @memberof ComputePipeline
 */
OBJECTIVELYGPU_EXPORT Class *_ComputePipeline(void);
