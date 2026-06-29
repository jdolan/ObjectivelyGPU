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
   * @brief The interface.
   * @protected
   */
  ComputePipelineInterface *interface;

  /**
   * @brief The underlying SDL compute pipeline.
   */
  SDL_GPUComputePipeline *pipeline;

  /**
   * @brief The RenderDevice that owns this pipeline.
   * @private
   */
  RenderDevice *device;
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
   * @details This is the designated initializer. All fields of @p info, including
   *   `code`, `code_size`, and `format`, must be set by the caller. Prefer
   *   `initWithResource` to load a compiled blob from the Resource system with
   *   automatic format selection.
   * @param self The ComputePipeline.
   * @param device The RenderDevice used to create and release the pipeline. Retained.
   * @param info Compute pipeline creation parameters with all fields populated.
   * @return The initialized ComputePipeline, or `NULL` on failure.
   * @memberof ComputePipeline
   */
  ComputePipeline *(*initWithDevice)(ComputePipeline *self, RenderDevice *device, const SDL_GPUComputePipelineCreateInfo *info);

  /**
   * @fn ComputePipeline *ComputePipeline::initWithResource(ComputePipeline *self, RenderDevice *device, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
   * @brief Initializes this ComputePipeline from a compiled blob loaded via the Resource system.
   * @details Appends the platform-appropriate extension to @p name and resolves it via
   *   Objectively's ResourceProvider chain:
   *   - Metal (macOS/iOS): `.msl`
   *   - Vulkan (Linux/Android): `.spv`
   *   - D3D12 (Windows): `.dxil`
   *   The caller fills in @c entrypoint, thread counts, and binding counts in @p info;
   *   `code`, `code_size`, and `format` are filled in here. Shader blobs are produced
   *   offline by @c sdl-shadercross.
   * @param self The ComputePipeline.
   * @param device The RenderDevice used to create and release the pipeline. Retained.
   * @param name Shader base name without extension, e.g. @c "HelloCompute.comp".
   * @param info Compute pipeline creation parameters; `code`, `code_size`, and `format` are ignored.
   * @return The initialized ComputePipeline. GPU_Asserts if no supported blob is found.
   * @memberof ComputePipeline
   */
  ComputePipeline *(*initWithResource)(ComputePipeline *self, RenderDevice *device, const char *name, const SDL_GPUComputePipelineCreateInfo *info);
};

/**
 * @fn Class *ComputePipeline::_ComputePipeline(void)
 * @brief The ComputePipeline archetype.
 * @return The ComputePipeline Class.
 * @memberof ComputePipeline
 */
OBJECTIVELYGPU_EXPORT Class *_ComputePipeline(void);
