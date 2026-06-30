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
#include <string.h>

#include "CommandBuffer.h"
#include "CopyPass.h"
#include "RenderDevice.h"
#include "Texture.h"

#define _Class _Texture

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Texture *this = (Texture *) self;

  if (this->device) {
    SDL_ReleaseGPUTexture(this->device->device, this->texture);
    release(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - Texture

/**
 * @fn Texture *Texture::initWithDevice(Texture *self, RenderDevice *device, const SDL_GPUTextureCreateInfo *info, const void *pixels)
 * @memberof Texture
 */
static Texture *initWithDevice(Texture *self, RenderDevice *device, const SDL_GPUTextureCreateInfo *info, const void *pixels) {

  assert(device);
  assert(info);

  self = (Texture *) super(Object, self, init);
  if (self) {

    self->device = retain(device);

    self->texture = SDL_CreateGPUTexture(device->device, info);
    GPU_Assert(self->texture, "SDL_CreateGPUTexture");

    self->format = info->format;
    self->size = MakeSize((int) info->width, (int) info->height);
    self->type = info->type;
    self->usage = info->usage;
    self->layerCountOrDepth = info->layer_count_or_depth;
    self->numLevels = info->num_levels;
    self->sampleCount = info->sample_count;

    if (pixels) {
      const Uint32 bytesPerTexel = SDL_GPUTextureFormatTexelBlockSize(info->format);
      const Uint32 totalBytes = info->width * info->height * info->layer_count_or_depth * bytesPerTexel;

      SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(device->device, &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = totalBytes,
      });
      GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

      void *mapped = SDL_MapGPUTransferBuffer(device->device, tbuf, false);
      GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

      memcpy(mapped, pixels, totalBytes);
      SDL_UnmapGPUTransferBuffer(device->device, tbuf);

      CommandBuffer *commands = $(device, acquireCommandBuffer);
      CopyPass *copyPass = $(commands, beginCopyPass);

      $(copyPass, uploadTexture,
        &(SDL_GPUTextureTransferInfo) {
          .transfer_buffer = tbuf,
          .pixels_per_row = info->width,
          .rows_per_layer = info->height,
        },
        &(SDL_GPUTextureRegion) {
          .texture = self->texture,
          .w = info->width,
          .h = info->height,
          .d = info->layer_count_or_depth,
        },
        false);

      release(copyPass);
      $(commands, submit);
      release(commands);
      SDL_ReleaseGPUTransferBuffer(device->device, tbuf);
    }
  }

  return self;
}

/**
 * @fn Texture *Texture::initWithSurface(Texture *self, RenderDevice *device, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage)
 * @memberof Texture
 */
static Texture *initWithSurface(Texture *self, RenderDevice *device, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage) {

  assert(device);
  assert(surface);

  SDL_Surface *rgba = surface->format == SDL_PIXELFORMAT_RGBA32
    ? surface
    : SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  GPU_Assert(rgba, "SDL_ConvertSurface");

  self = $(self, initWithDevice, device, &(SDL_GPUTextureCreateInfo) {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = usage,
    .width = (Uint32) rgba->w,
    .height = (Uint32) rgba->h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  }, rgba->pixels);

  if (rgba != surface) {
    SDL_DestroySurface(rgba);
  }

  return self;
}

/**
 * @fn void Texture::setName(Texture *self, const char *name)
 * @memberof Texture
 */
static void setName(Texture *self, const char *name) {

  assert(self);

  SDL_SetGPUTextureName(self->device->device, self->texture, name);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((TextureInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((TextureInterface *) clazz->interface)->initWithSurface = initWithSurface;
  ((TextureInterface *) clazz->interface)->setName = setName;
}

/**
 * @fn Class *Texture::_Texture(void)
 * @memberof Texture
 */
Class *_Texture(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "Texture",
      .superclass      = _Object(),
      .instanceSize    = sizeof(Texture),
      .interfaceOffset = offsetof(Texture, interface),
      .interfaceSize   = sizeof(TextureInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
