/*
 * ObjectivelyGPU: Object oriented MVC framework for SDL3 and GNU C.
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

#include "Framebuffer.h"
#include "RenderDevice.h"

#define _Class _Framebuffer

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Framebuffer *this = (Framebuffer *) self;

  $(this->device, releaseTexture, this->colorTexture);
  $(this->device, releaseTexture, this->depthTexture);

  release(this->device);

  super(Object, self, dealloc);
}

#pragma mark - Framebuffer

/**
 * @brief Allocates color and depth textures for the current size and formats.
 */
static void allocateTextures(Framebuffer *this) {

  if (this->colorFormat != SDL_GPU_TEXTUREFORMAT_INVALID) {
    this->colorTexture = $(this->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = this->colorFormat,
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
      .width = (Uint32) this->size.w,
      .height = (Uint32) this->size.h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
    }, NULL);
  }

  if (this->depthFormat != SDL_GPU_TEXTUREFORMAT_INVALID) {
    this->depthTexture = $(this->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = this->depthFormat,
      .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
      .width = (Uint32) this->size.w,
      .height = (Uint32) this->size.h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = SDL_GPU_SAMPLECOUNT_1,
    }, NULL);
  }
}

/**
 * @fn SDL_GPUColorTargetInfo Framebuffer::colorTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor)
 * @memberof Framebuffer
 */
static SDL_GPUColorTargetInfo colorTargetInfo(const Framebuffer *self,
  SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor) {

  assert(self->colorTexture);

  return (SDL_GPUColorTargetInfo) {
    .texture = self->colorTexture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_color = clearColor ? *clearColor : (SDL_FColor) { 0.f, 0.f, 0.f, 1.f },
  };
}

/**
 * @fn SDL_GPUDepthStencilTargetInfo Framebuffer::depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, float clearDepth)
 * @memberof Framebuffer
 */
static SDL_GPUDepthStencilTargetInfo depthTargetInfo(const Framebuffer *self,
  SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, float clearDepth) {

  assert(self->depthTexture);

  return (SDL_GPUDepthStencilTargetInfo) {
    .texture = self->depthTexture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_depth = clearDepth,
  };
}

/**
 * @fn Framebuffer *Framebuffer::initWithDevice(Framebuffer *self, RenderDevice *device, const SDL_Size *size, SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat)
 * @memberof Framebuffer
 */
static Framebuffer *initWithDevice(Framebuffer *self, RenderDevice *device,
  const SDL_Size *size,
  SDL_GPUTextureFormat colorFormat,
  SDL_GPUTextureFormat depthFormat) {

  assert(device);
  assert(size);
  assert(size->w > 0 && size->h > 0);

  self = (Framebuffer *) super(Object, self, init);
  if (self) {
    self->device = retain(device);
    self->size = *size;
    self->colorFormat = colorFormat;
    self->depthFormat = depthFormat;

    allocateTextures(self);
  }

  return self;
}

/**
 * @fn bool Framebuffer::resize(Framebuffer *self, const SDL_Size *size)
 * @memberof Framebuffer
 */
static bool resize(Framebuffer *self, const SDL_Size *size) {
  assert(self);
  assert(size);
  assert(size->w > 0 && size->h > 0);

  if (self->size.w == size->w && self->size.h == size->h) {
    return false;
  }

  $(self->device, releaseTexture, self->colorTexture);
  self->colorTexture = NULL;

  $(self->device, releaseTexture, self->depthTexture);
  self->depthTexture = NULL;

  self->size = *size;
  allocateTextures(self);

  return true;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((FramebufferInterface *) clazz->interface)->colorTargetInfo = colorTargetInfo;
  ((FramebufferInterface *) clazz->interface)->depthTargetInfo = depthTargetInfo;
  ((FramebufferInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((FramebufferInterface *) clazz->interface)->resize = resize;
}

/**
 * @fn Class *Framebuffer::_Framebuffer(void)
 * @memberof Framebuffer
 */
Class *_Framebuffer(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "Framebuffer",
      .superclass      = _Object(),
      .instanceSize    = sizeof(Framebuffer),
      .interfaceOffset = offsetof(Framebuffer, interface),
      .interfaceSize   = sizeof(FramebufferInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
