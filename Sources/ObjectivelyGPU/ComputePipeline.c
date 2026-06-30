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

#include <Objectively/Resource.h>

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
    assert(self->device);

    self->pipeline = SDL_CreateGPUComputePipeline(device->device, info);
    GPU_Assert(self->pipeline, "SDL_CreateGPUComputePipeline");
  }

  return self;
}

/**
 * @fn ComputePipeline *ComputePipeline::initWithResource(ComputePipeline *self, RenderDevice *device, const Resource *resource, SDL_GPUShaderFormat format, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof ComputePipeline
 */
static ComputePipeline *initWithResource(ComputePipeline *self, RenderDevice *device, const Resource *resource, SDL_GPUShaderFormat format, const SDL_GPUComputePipelineCreateInfo *info) {

  assert(device);
  assert(resource && resource->data && resource->data->length);
  assert(info);

  SDL_GPUComputePipelineCreateInfo create = *info;
  create.code = resource->data->bytes;
  create.code_size = resource->data->length;
  create.format = format;
  create.entrypoint = create.entrypoint ?: (format == SDL_GPU_SHADERFORMAT_MSL) ? "main0" : "main";

  return $(self, initWithDevice, device, &create);
}

/**
 * @fn ComputePipeline *ComputePipeline::initWithResourceName(ComputePipeline *self, RenderDevice *device, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof ComputePipeline
 */
static ComputePipeline *initWithResourceName(ComputePipeline *self, RenderDevice *device, const char *name, const SDL_GPUComputePipelineCreateInfo *info) {

  assert(device);
  assert(name);
  assert(info);

  static const struct {
    SDL_GPUShaderFormat format;
    const char *ext;
  } formats[] = {
    { SDL_GPU_SHADERFORMAT_MSL,  ".metal"  },
    { SDL_GPU_SHADERFORMAT_DXIL, ".dxil" },
    { SDL_GPU_SHADERFORMAT_SPIRV,".spv"  },
  };

  const SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(device->device);

  for (size_t i = 0; i < SDL_arraysize(formats); i++) {

    if (!(supported & formats[i].format)) {
      continue;
    }

    char path[256];
    SDL_snprintf(path, sizeof(path), "%s%s", name, formats[i].ext);

    Resource *res = $$(Resource, resourceWithName, path);
    if (!res) {
      continue;
    }

    if (!res->data || res->data->length == 0) {
      release(res);
      continue;
    }

    self = $(self, initWithResource, device, res, formats[i].format, info);
    release(res);
    return self;
  }

  GPU_Assert(false, "no supported shader format found for '%s'", name);
  return NULL;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((ComputePipelineInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((ComputePipelineInterface *) clazz->interface)->initWithResource = initWithResource;
  ((ComputePipelineInterface *) clazz->interface)->initWithResourceName = initWithResourceName;
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
