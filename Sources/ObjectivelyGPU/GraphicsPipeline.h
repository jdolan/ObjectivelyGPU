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
 * @brief GraphicsPipeline wraps an `SDL_GPUGraphicsPipeline`, owning its handle.
 */

typedef struct RenderDevice RenderDevice;
typedef struct GraphicsPipeline GraphicsPipeline;
typedef struct GraphicsPipelineInterface GraphicsPipelineInterface;

/**
 * @brief An `SDL_GPUGraphicsPipeline`: the compiled vertex/fragment program plus
 * all fixed-function render state (blend, rasteriser, depth/stencil, targets).
 *
 * GraphicsPipeline owns its underlying `SDL_GPUGraphicsPipeline` and releases it
 * in `dealloc`, so a pipeline is freed with `release` like any other Objectively
 * object. Bind it with `RenderPass::bindPipeline` via its `pipeline` handle.
 *
 * @extends Object
 */
struct GraphicsPipeline {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  GraphicsPipelineInterface *interface;

  /**
   * @brief The underlying SDL graphics pipeline.
   */
  SDL_GPUGraphicsPipeline *pipeline;

  /**
   * @brief The RenderDevice that owns this pipeline.
   * @private
   */
  RenderDevice *device;
};

/**
 * @brief The GraphicsPipeline interface.
 */
struct GraphicsPipelineInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn GraphicsPipeline *GraphicsPipeline::initWithDevice(GraphicsPipeline *self, RenderDevice *device, const SDL_GPUGraphicsPipelineCreateInfo *info)
   * @brief Initializes this GraphicsPipeline, compiling it from @p info.
   * @details This is the designated initializer. All shaders, vertex input state,
   *   blend, rasteriser, depth/stencil state, and colour target formats are supplied
   *   in @p info.
   * @param self The GraphicsPipeline.
   * @param device The RenderDevice used to create and release the pipeline. Retained.
   * @param info Graphics pipeline creation parameters.
   * @return The initialized GraphicsPipeline, or `NULL` on failure.
   * @memberof GraphicsPipeline
   */
  GraphicsPipeline *(*initWithDevice)(GraphicsPipeline *self, RenderDevice *device, const SDL_GPUGraphicsPipelineCreateInfo *info);
};

/**
 * @fn Class *GraphicsPipeline::_GraphicsPipeline(void)
 * @brief The GraphicsPipeline archetype.
 * @return The GraphicsPipeline Class.
 * @memberof GraphicsPipeline
 */
OBJECTIVELYGPU_EXPORT Class *_GraphicsPipeline(void);

/**
 * @defgroup GraphicsPipelinePresets Graphics pipeline presets
 * @brief Ready-made render state to copy into an `SDL_GPUGraphicsPipelineCreateInfo`.
 *
 * Copy a pipeline preset by value, then fill in the caller-specific fields —
 * `vertex_shader`, `fragment_shader`, `vertex_input_state`, and `target_info`:
 *
 * @code
 *   SDL_GPUGraphicsPipelineCreateInfo info = GPU_GraphicsPipeline3D;
 *   info.vertex_shader = vertexShader->shader;
 *   info.fragment_shader = fragmentShader->shader;
 *   info.vertex_input_state = (SDL_GPUVertexInputState) { ... };
 *   info.target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
 *     .color_target_descriptions = &(SDL_GPUColorTargetDescription) {
 *       .format = framebuffer->colorTexture->format,
 *       .blend_state = GPU_BlendStateOpaque,
 *     },
 *     .num_color_targets = 1,
 *     .depth_stencil_format = framebuffer->depthTexture->format,
 *     .has_depth_stencil_target = true,
 *   };
 *   GraphicsPipeline *pipeline = $(renderDevice, createGraphicsPipeline, &info);
 * @endcode
 *
 * @{
 */

/**
 * @brief Opaque blending: source color overwrites the destination (no blending).
 */
OBJECTIVELYGPU_EXPORT const SDL_GPUColorTargetBlendState GPU_BlendStateOpaque;

/**
 * @brief Straight (non-premultiplied) alpha blending: `src·srcA + dst·(1 - srcA)`.
 */
OBJECTIVELYGPU_EXPORT const SDL_GPUColorTargetBlendState GPU_BlendStateAlpha;

/**
 * @brief Premultiplied alpha blending: `src + dst·(1 - srcA)`.
 */
OBJECTIVELYGPU_EXPORT const SDL_GPUColorTargetBlendState GPU_BlendStatePremultipliedAlpha;

/**
 * @brief Additive blending: `src·srcA + dst`, for glow, particles, and light accumulation.
 */
OBJECTIVELYGPU_EXPORT const SDL_GPUColorTargetBlendState GPU_BlendStateAdditive;

/**
 * @brief 3D world geometry: triangle list, solid fill, back-face culling (CCW front),
 *   depth test and write enabled with `LESS_OR_EQUAL`.
 * @details Fill in `vertex_shader`, `fragment_shader`, `vertex_input_state`, and
 *   `target_info` (including `depth_stencil_format` and `has_depth_stencil_target`).
 */
OBJECTIVELYGPU_EXPORT const SDL_GPUGraphicsPipelineCreateInfo GPU_GraphicsPipeline3D;

/**
 * @brief 2D UI and sprites: triangle list, solid fill, no culling, no depth test.
 * @details Fill in `vertex_shader`, `fragment_shader`, `vertex_input_state`, and
 *   `target_info`. Apply a blend preset via each color target's `blend_state`.
 */
OBJECTIVELYGPU_EXPORT const SDL_GPUGraphicsPipelineCreateInfo GPU_GraphicsPipeline2D;

/**
 * @}
 */
