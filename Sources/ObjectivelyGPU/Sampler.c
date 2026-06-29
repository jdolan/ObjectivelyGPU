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

#include <assert.h>

#include "RenderDevice.h"
#include "Sampler.h"

#define _Class _Sampler

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Sampler *this = (Sampler *) self;

  if (this->device) {
    SDL_ReleaseGPUSampler(this->device->device, this->sampler);
    release(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - Sampler

/**
 * @fn Sampler *Sampler::initWithDevice(Sampler *self, RenderDevice *device, const SDL_GPUSamplerCreateInfo *info)
 * @memberof Sampler
 */
static Sampler *initWithDevice(Sampler *self, RenderDevice *device, const SDL_GPUSamplerCreateInfo *info) {

  assert(device);
  assert(info);

  self = (Sampler *) super(Object, self, init);
  if (self) {

    self->device = retain(device);

    self->sampler = SDL_CreateGPUSampler(device->device, info);
    GPU_Assert(self->sampler, "SDL_CreateGPUSampler");
  }

  return self;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((SamplerInterface *) clazz->interface)->initWithDevice = initWithDevice;
}

/**
 * @fn Class *Sampler::_Sampler(void)
 * @memberof Sampler
 */
Class *_Sampler(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "Sampler",
      .superclass      = _Object(),
      .instanceSize    = sizeof(Sampler),
      .interfaceOffset = offsetof(Sampler, interface),
      .interfaceSize   = sizeof(SamplerInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
