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

#include "RenderDevice.h"
#include "Shader.h"

#define _Class _Shader

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Shader *this = (Shader *) self;

  if (this->device) {
    SDL_ReleaseGPUShader(this->device->device, this->shader);
  }

  super(Object, self, dealloc);
}

#pragma mark - Shader

/**
 * @fn Shader *Shader::initWithDevice(Shader *self, RenderDevice *device, const SDL_GPUShaderCreateInfo *info)
 * @memberof Shader
 */
static Shader *initWithDevice(Shader *self, RenderDevice *device, const SDL_GPUShaderCreateInfo *info) {

  assert(device);
  assert(info);

  self = (Shader *) super(Object, self, init);
  if (self) {
    self->device = device;

    self->stage = info->stage;
    self->format = info->format;

    self->shader = SDL_CreateGPUShader(device->device, info);
    GPU_Assert(self->shader, "SDL_CreateGPUShader");
  }

  return self;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((ShaderInterface *) clazz->interface)->initWithDevice = initWithDevice;
}

/**
 * @fn Class *Shader::_Shader(void)
 * @memberof Shader
 */
Class *_Shader(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "Shader",
      .superclass      = _Object(),
      .instanceSize    = sizeof(Shader),
      .interfaceSize   = sizeof(ShaderInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
