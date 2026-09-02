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

#include "Fence.h"
#include "RenderDevice.h"

#define _Class _Fence

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Fence *this = (Fence *) self;

  if (this->device) {
    if (this->fence) {
      SDL_ReleaseGPUFence(this->device->device, this->fence);
    }
  }

  super(Object, self, dealloc);
}

#pragma mark - Fence

/**
 * @fn Fence *Fence::initWithDevice(Fence *self, RenderDevice *device, SDL_GPUFence *fence)
 * @memberof Fence
 */
static Fence *initWithDevice(Fence *self, RenderDevice *device, SDL_GPUFence *fence) {

  assert(device);
  assert(fence);

  self = (Fence *) super(Object, self, init);
  if (self) {
    self->device = device;
    self->fence = fence;
  }

  return self;
}

/**
 * @fn bool Fence::query(const Fence *self)
 * @memberof Fence
 */
static bool query(const Fence *self) {
  return SDL_QueryGPUFence(self->device->device, self->fence);
}

/**
 * @fn bool Fence::wait(const Fence *self)
 * @memberof Fence
 */
static bool _wait(const Fence *self) {
  return SDL_WaitForGPUFences(self->device->device, true, &self->fence, 1);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((FenceInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((FenceInterface *) clazz->interface)->query = query;
  ((FenceInterface *) clazz->interface)->wait = _wait;
}

/**
 * @fn Class *Fence::_Fence(void)
 * @memberof Fence
 */
Class *_Fence(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "Fence",
      .superclass      = _Object(),
      .instanceSize    = sizeof(Fence),
      .interfaceSize   = sizeof(FenceInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
