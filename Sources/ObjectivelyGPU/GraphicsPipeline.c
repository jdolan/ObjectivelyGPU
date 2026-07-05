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

#include <assert.h>

#include "GraphicsPipeline.h"
#include "RenderDevice.h"

#define _Class _GraphicsPipeline

#pragma mark - Presets

OBJECTIVELYGPU_EXPORT_DATA const SDL_GPUColorTargetBlendState GPU_BlendStateOpaque = {
  .enable_blend = false,
};

OBJECTIVELYGPU_EXPORT_DATA const SDL_GPUColorTargetBlendState GPU_BlendStateAlpha = {
  .enable_blend = true,
  .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
  .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .color_blend_op = SDL_GPU_BLENDOP_ADD,
  .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
  .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
};

OBJECTIVELYGPU_EXPORT_DATA const SDL_GPUColorTargetBlendState GPU_BlendStatePremultipliedAlpha = {
  .enable_blend = true,
  .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
  .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .color_blend_op = SDL_GPU_BLENDOP_ADD,
  .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
  .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
  .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
};

OBJECTIVELYGPU_EXPORT_DATA const SDL_GPUColorTargetBlendState GPU_BlendStateAdditive = {
  .enable_blend = true,
  .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
  .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
  .color_blend_op = SDL_GPU_BLENDOP_ADD,
  .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
  .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
  .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
};

OBJECTIVELYGPU_EXPORT_DATA const SDL_GPUGraphicsPipelineCreateInfo GPU_GraphicsPipeline3D = {
  .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
  .rasterizer_state = {
    .fill_mode = SDL_GPU_FILLMODE_FILL,
    .cull_mode = SDL_GPU_CULLMODE_BACK,
    .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
    .enable_depth_clip = true,
  },
  .multisample_state = {
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  },
  .depth_stencil_state = {
    .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
    .enable_depth_test = true,
    .enable_depth_write = true,
  },
};

OBJECTIVELYGPU_EXPORT_DATA const SDL_GPUGraphicsPipelineCreateInfo GPU_GraphicsPipeline2D = {
  .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
  .rasterizer_state = {
    .fill_mode = SDL_GPU_FILLMODE_FILL,
    .cull_mode = SDL_GPU_CULLMODE_NONE,
    .enable_depth_clip = true,
  },
  .multisample_state = {
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  },
};

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  GraphicsPipeline *this = (GraphicsPipeline *) self;

  if (this->device) {
    SDL_ReleaseGPUGraphicsPipeline(this->device->device, this->pipeline);
    release(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - GraphicsPipeline

/**
 * @fn GraphicsPipeline *GraphicsPipeline::initWithDevice(GraphicsPipeline *self, RenderDevice *device, const SDL_GPUGraphicsPipelineCreateInfo *info)
 * @memberof GraphicsPipeline
 */
static GraphicsPipeline *initWithDevice(GraphicsPipeline *self, RenderDevice *device, const SDL_GPUGraphicsPipelineCreateInfo *info) {

  assert(device);
  assert(info);

  self = (GraphicsPipeline *) super(Object, self, init);
  if (self) {

    self->device = retain(device);

    SDL_Log("Assembling GraphicsPipeline (%u color targets, %ux MSAA, %s)",
            info->target_info.num_color_targets,
            info->multisample_state.sample_count == SDL_GPU_SAMPLECOUNT_1 ? 1 :
                info->multisample_state.sample_count == SDL_GPU_SAMPLECOUNT_2 ? 2 :
                info->multisample_state.sample_count == SDL_GPU_SAMPLECOUNT_4 ? 4 : 8,
            info->target_info.has_depth_stencil_target ? "depth" : "no depth");

    self->pipeline = SDL_CreateGPUGraphicsPipeline(device->device, info);
    GPU_Assert(self->pipeline, "SDL_CreateGPUGraphicsPipeline");
  }

  return self;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((GraphicsPipelineInterface *) clazz->interface)->initWithDevice = initWithDevice;
}

/**
 * @fn Class *GraphicsPipeline::_GraphicsPipeline(void)
 * @memberof GraphicsPipeline
 */
Class *_GraphicsPipeline(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "GraphicsPipeline",
      .superclass      = _Object(),
      .instanceSize    = sizeof(GraphicsPipeline),
      .interfaceOffset = offsetof(GraphicsPipeline, interface),
      .interfaceSize   = sizeof(GraphicsPipelineInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
