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

#include <Objectively/Resource.h>

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
    release(this->device);
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

    self->device = retain(device);

    self->shader = SDL_CreateGPUShader(device->device, info);
    GPU_Assert(self->shader, "SDL_CreateGPUShader");

    self->stage = info->stage;
  }

  return self;
}

/**
 * @fn Shader *Shader::initWithResource(Shader *self, RenderDevice *device, const char *name, const SDL_GPUShaderCreateInfo *info)
 * @memberof Shader
 */
static Shader *initWithResource(Shader *self, RenderDevice *device, const char *name, const SDL_GPUShaderCreateInfo *info) {

  assert(device);
  assert(name);
  assert(info);

  static const struct {
    SDL_GPUShaderFormat format;
    const char *ext;
  } formats[] = {
    { SDL_GPU_SHADERFORMAT_MSL,  ".msl"  },
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

    SDL_GPUShaderCreateInfo filled = *info;
    filled.code = res->data->bytes;
    filled.code_size = res->data->length;
    filled.format = formats[i].format;

    if (!filled.entrypoint) {
      filled.entrypoint = (formats[i].format == SDL_GPU_SHADERFORMAT_MSL) ? "main0" : "main";
    }

    self = $(self, initWithDevice, device, &filled);
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

  ((ShaderInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((ShaderInterface *) clazz->interface)->initWithResource = initWithResource;
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
      .interfaceOffset = offsetof(Shader, interface),
      .interfaceSize   = sizeof(ShaderInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
