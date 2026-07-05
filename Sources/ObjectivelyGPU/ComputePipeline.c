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

#include "ComputePipeline.h"
#include "RenderDevice.h"

#define _Class _ComputePipeline

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  ComputePipeline *this = (ComputePipeline *) self;

  if (this->device) {
    SDL_ReleaseGPUComputePipeline(this->device->device, this->pipeline);
    release(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - ComputePipeline

/**
 * @fn ComputePipeline *ComputePipeline::initWithDevice(ComputePipeline *self, RenderDevice *device, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof ComputePipeline
 */
static ComputePipeline *initWithDevice(ComputePipeline *self, RenderDevice *device, const SDL_GPUComputePipelineCreateInfo *info) {

  assert(device);
  assert(info);

  self = (ComputePipeline *) super(Object, self, init);
  if (self) {
    self->device = retain(device);

    SDL_Log("Assembling ComputePipeline (threads %ux%ux%u, %u samplers, %u storage textures, %u storage buffers)",
            info->threadcount_x, info->threadcount_y, info->threadcount_z,
            info->num_samplers,
            info->num_readonly_storage_textures + info->num_readwrite_storage_textures,
            info->num_readonly_storage_buffers + info->num_readwrite_storage_buffers);

    self->pipeline = SDL_CreateGPUComputePipeline(device->device, info);
    GPU_Assert(self->pipeline, "SDL_CreateGPUComputePipeline");
  }

  return self;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((ComputePipelineInterface *) clazz->interface)->initWithDevice = initWithDevice;
}

/**
 * @fn Class *ComputePipeline::_ComputePipeline(void)
 * @memberof ComputePipeline
 */
Class *_ComputePipeline(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "ComputePipeline",
      .superclass      = _Object(),
      .instanceSize    = sizeof(ComputePipeline),
      .interfaceOffset = offsetof(ComputePipeline, interface),
      .interfaceSize   = sizeof(ComputePipelineInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
