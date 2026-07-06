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
#include <math.h>
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
 * @brief Uploads @p size bytes of pixel data into mip level 0 of this texture.
 * @details The source data is laid out at @p pixelsPerRow pixels (texels) per row, so
 *   callers can pass strided data (e.g. an `SDL_Surface` with row padding) without first
 *   tightening it. The texture's dimensions and layer count come from its metadata, so
 *   this must be called after `initWithDevice` has populated them.
 */
static void uploadPixels(Texture *self, const void *pixels, Uint32 size, Uint32 pixelsPerRow) {

  SDL_GPUDevice *device = self->device->device;

  SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo) {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = size,
  });
  GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

  void *mapped = SDL_MapGPUTransferBuffer(device, tbuf, false);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  memcpy(mapped, pixels, size);
  SDL_UnmapGPUTransferBuffer(device, tbuf);

  CommandBuffer *commands = $(self->device, acquireCommandBuffer);
  CopyPass *copyPass = $(commands, beginCopyPass);

  if (self->type == SDL_GPU_TEXTURETYPE_3D) {

    // A 3D texture's slices are addressed by depth (region.z / .d), not by layer,
    // so upload the whole volume in one region.
    $(copyPass, uploadTexture,
      &(SDL_GPUTextureTransferInfo) {
        .transfer_buffer = tbuf,
        .pixels_per_row = pixelsPerRow,
        .rows_per_layer = (Uint32) self->size.h,
      },
      &(SDL_GPUTextureRegion) {
        .texture = self->texture,
        .w = (Uint32) self->size.w,
        .h = (Uint32) self->size.h,
        .d = self->layerCountOrDepth,
      },
      false);
  } else {

    // Everything else (2D, 2D array, cube) is addressed by layer: upload one layer
    // per call from its layer-major slice of the transfer buffer. A single region
    // with d = layerCount only populates layer 0, so we must loop; a plain 2D
    // texture is simply one iteration.
    const Uint32 layers = self->layerCountOrDepth ? self->layerCountOrDepth : 1;
    const Uint32 layerBytes = size / layers;

    for (Uint32 layer = 0; layer < layers; layer++) {
      $(copyPass, uploadTexture,
        &(SDL_GPUTextureTransferInfo) {
          .transfer_buffer = tbuf,
          .offset = layer * layerBytes,
          .pixels_per_row = pixelsPerRow,
          .rows_per_layer = (Uint32) self->size.h,
        },
        &(SDL_GPUTextureRegion) {
          .texture = self->texture,
          .layer = layer,
          .w = (Uint32) self->size.w,
          .h = (Uint32) self->size.h,
          .d = 1,
        },
        false);
    }
  }

  release(copyPass);
  $(commands, submit);
  release(commands);
  SDL_ReleaseGPUTransferBuffer(device, tbuf);
}

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
      const Uint32 size = SDL_CalculateGPUTextureFormatSize(info->format, info->width, info->height, info->layer_count_or_depth);
      uploadPixels(self, pixels, size, info->width);
    }
  }

  return self;
}

/**
 * @fn Texture *Texture::initWithSurface(Texture *self, RenderDevice *device, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage, bool generateMipmaps)
 * @memberof Texture
 */
static Texture *initWithSurface(Texture *self, RenderDevice *device, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage, bool generateMipmaps) {

  assert(device);
  assert(surface);

  SDL_Surface *rgba = surface->format == SDL_PIXELFORMAT_RGBA32
    ? surface
    : SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  GPU_Assert(rgba, "SDL_ConvertSurface");

  const Uint32 numLevels = generateMipmaps ? (Uint32) (floorf(log2f((float) SDL_min(rgba->w, rgba->h))) + 1) : 1;
  if (generateMipmaps) {
    usage |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
  }

  // Create the texture empty, then upload respecting the surface's row pitch — an
  // SDL_Surface's pitch is often larger than width * bytesPerPixel (alignment, or a
  // sub-surface of an atlas), so assuming tightly-packed rows would skew the image.
  self = $(self, initWithDevice, device, &(SDL_GPUTextureCreateInfo) {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = usage,
    .width = (Uint32) rgba->w,
    .height = (Uint32) rgba->h,
    .layer_count_or_depth = 1,
    .num_levels = numLevels,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  }, NULL);

  if (self) {
    const Uint32 bytesPerPixel = SDL_BYTESPERPIXEL(rgba->format);
    uploadPixels(self, rgba->pixels,
      (Uint32) rgba->pitch * (Uint32) rgba->h,
      (Uint32) rgba->pitch / bytesPerPixel);

    if (generateMipmaps) {
      // Mipmap generation is not a copy-pass operation and must run outside any pass,
      // so it gets its own one-shot command buffer after the base level upload above.
      CommandBuffer *commands = $(device, acquireCommandBuffer);
      $(commands, generateMipmaps, self->texture);
      $(commands, submit);
      release(commands);
    }
  }

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

/**
 * @fn void *Texture::downloadPixels(const Texture *self)
 * @memberof Texture
 */
static void *downloadPixels(const Texture *self) {

  assert(self);

  SDL_GPUDevice *device = self->device->device;

  const Uint32 size = SDL_CalculateGPUTextureFormatSize(self->format, (Uint32) self->size.w, (Uint32) self->size.h, 1);

  SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo) {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
    .size = size,
  });
  GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

  CommandBuffer *commands = $(self->device, acquireCommandBuffer);
  CopyPass *copyPass = $(commands, beginCopyPass);

  $(copyPass, downloadTexture,
    &(SDL_GPUTextureRegion) {
      .texture = self->texture,
      .w = (Uint32) self->size.w,
      .h = (Uint32) self->size.h,
      .d = 1,
    },
    &(SDL_GPUTextureTransferInfo) {
      .transfer_buffer = tbuf,
      .pixels_per_row = (Uint32) self->size.w,
      .rows_per_layer = (Uint32) self->size.h,
    });

  release(copyPass);

  // Downloads are asynchronous: block on a fence before mapping the transfer
  // buffer, so the caller always sees complete data. Not suited to per-frame use.
  SDL_GPUFence *fence = $(commands, submitAndFence);
  release(commands);
  $(self->device, waitForFences, true, &fence, 1);
  $(self->device, releaseFence, fence);

  void *mapped = SDL_MapGPUTransferBuffer(device, tbuf, false);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  void *pixels = malloc(size);
  memcpy(pixels, mapped, size);

  SDL_UnmapGPUTransferBuffer(device, tbuf);
  SDL_ReleaseGPUTransferBuffer(device, tbuf);

  return pixels;
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
  ((TextureInterface *) clazz->interface)->downloadPixels = downloadPixels;
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
