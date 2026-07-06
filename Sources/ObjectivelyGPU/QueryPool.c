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

#include "QueryPool.h"
#include "RenderDevice.h"

#define _Class _QueryPool

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  QueryPool *this = (QueryPool *) self;

  if (this->device) {
#ifdef SDL_GPU_OCCLUSION_QUERY
    if (this->pool) {
      SDL_ReleaseGPUQueryPool(this->device->device, this->pool);
    }
#endif
    release(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - QueryPool

/**
 * @fn QueryPool *QueryPool::initWithDevice(QueryPool *self, RenderDevice *device, const SDL_GPUQueryPoolCreateInfo *info)
 * @memberof QueryPool
 */
static QueryPool *initWithDevice(QueryPool *self, RenderDevice *device, const SDL_GPUQueryPoolCreateInfo *info) {

  assert(device);
  assert(info);

  self = (QueryPool *) super(Object, self, init);
  if (self) {

    self->device = retain(device);
    self->type = info->type;
    self->queryCount = info->query_count;

#ifdef SDL_GPU_OCCLUSION_QUERY
    self->pool = SDL_CreateGPUQueryPool(device->device, (SDL_GPUQueryPoolCreateInfo *) info);
    GPU_Assert(self->pool, "SDL_CreateGPUQueryPool");
#else
    self->pool = NULL; // Occlusion queries are not supported by this SDL_gpu build.
#endif
  }

  return self;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((QueryPoolInterface *) clazz->interface)->initWithDevice = initWithDevice;
}

/**
 * @fn Class *QueryPool::_QueryPool(void)
 * @memberof QueryPool
 */
Class *_QueryPool(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "QueryPool",
      .superclass      = _Object(),
      .instanceSize    = sizeof(QueryPool),
      .interfaceOffset = offsetof(QueryPool, interface),
      .interfaceSize   = sizeof(QueryPoolInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
