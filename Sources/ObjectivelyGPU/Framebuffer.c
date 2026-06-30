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

#include "Framebuffer.h"
#include "RenderDevice.h"
#include "Texture.h"

#define _Class _Framebuffer

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Framebuffer *this = (Framebuffer *) self;

  release(this->colorTexture);
  release(this->resolveTexture);
  release(this->depthTexture);

  release(this->device);

  super(Object, self, dealloc);
}

#pragma mark - Framebuffer

/**
 * @fn SDL_GPUColorTargetInfo Framebuffer::colorTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor)
 * @memberof Framebuffer
 */
static SDL_GPUColorTargetInfo colorTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor) {

  assert(self->colorTexture);

  SDL_GPUColorTargetInfo info = {
    .texture = self->colorTexture->texture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_color = clearColor ? *clearColor : (SDL_FColor) { 0.f, 0.f, 0.f, 1.f },
  };

  if (self->sampleCount > SDL_GPU_SAMPLECOUNT_1) {
    assert(self->resolveTexture);

    // Resolve the multisampled color into the single-sample resolve target. STORE is
    // promoted to RESOLVE_AND_STORE so the multisampled contents survive for any later
    // load-op pass (e.g. UI drawn over a 3D scene) while keeping the resolve current.
    info.resolve_texture = self->resolveTexture->texture;
    if (storeOp == SDL_GPU_STOREOP_STORE) {
      info.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
    }
  }

  return info;
}

/**
 * @fn SDL_GPUDepthStencilTargetInfo Framebuffer::depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, float clearDepth)
 * @memberof Framebuffer
 */
static SDL_GPUDepthStencilTargetInfo depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, float clearDepth) {

  assert(self->depthTexture);

  return (SDL_GPUDepthStencilTargetInfo) {
    .texture = self->depthTexture->texture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_depth = clearDepth,
  };
}

/**
 * @fn Framebuffer *Framebuffer::initWithDevice(Framebuffer *self, RenderDevice *device, const GPU_FramebufferCreateInfo *info)
 * @memberof Framebuffer
 */
static Framebuffer *initWithDevice(Framebuffer *self, RenderDevice *device, const GPU_FramebufferCreateInfo *info) {

  assert(device);
  assert(info && info->size.w > 0 && info->size.h > 0);

  self = (Framebuffer *) super(Object, self, init);
  if (self) {
    self->device = retain(device);

    self->colorFormat = info->colorFormat;
    self->depthFormat = info->depthFormat;
    self->sampleCount = info->sampleCount;

    $(self, resize, &info->size);
  }

  return self;
}

/**
 * @fn bool Framebuffer::resize(Framebuffer *self, const SDL_Size *size)
 * @memberof Framebuffer
 */
static bool resize(Framebuffer *self, const SDL_Size *size) {

  assert(size);

  if (self->size.w == size->w && self->size.h == size->h) {
    return false;
  }

  release(self->colorTexture);
  release(self->resolveTexture);
  release(self->depthTexture);
  self->colorTexture = NULL;
  self->resolveTexture = NULL;
  self->depthTexture = NULL;

  self->size = *size;

  const bool multisampled = self->sampleCount > SDL_GPU_SAMPLECOUNT_1;

  if (self->colorFormat != SDL_GPU_TEXTUREFORMAT_INVALID) {

    // A multisampled color target is resolved before sampling, so it needs only
    // COLOR_TARGET usage; the single-sample paths (no MSAA, or the resolve target)
    // also carry SAMPLER so they can be blit/sampled.
    self->colorTexture = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = self->colorFormat,
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | (multisampled ? 0 : SDL_GPU_TEXTUREUSAGE_SAMPLER),
      .width = (Uint32) self->size.w,
      .height = (Uint32) self->size.h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = self->sampleCount,
    }, NULL);

    if (multisampled) {
      self->resolveTexture = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = self->colorFormat,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = (Uint32) self->size.w,
        .height = (Uint32) self->size.h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
      }, NULL);
    }
  }

  if (self->depthFormat != SDL_GPU_TEXTUREFORMAT_INVALID) {
    self->depthTexture = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = self->depthFormat,
      .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
      .width = (Uint32) self->size.w,
      .height = (Uint32) self->size.h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = self->sampleCount,
    }, NULL);
  }

  return true;
}

/**
 * @fn Texture *Framebuffer::resolvedColorTexture(const Framebuffer *self)
 * @memberof Framebuffer
 */
static Texture *resolvedColorTexture(const Framebuffer *self) {
  return self->resolveTexture ?: self->colorTexture;
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
  ((FramebufferInterface *) clazz->interface)->resolvedColorTexture = resolvedColorTexture;
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
