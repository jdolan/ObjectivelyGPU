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

  for (Uint32 i = 0; i < this->numColorTargets; i++) {
    release(this->colorTextures[i]);
    release(this->resolveTextures[i]);
  }
  release(this->depthTexture);
  release(this->resolveDepthTexture);

  release(this->device);

  super(Object, self, dealloc);
}

#pragma mark - Framebuffer

/**
 * @fn SDL_GPUColorTargetInfo Framebuffer::colorTargetInfo(const Framebuffer *self, Uint32 index, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor)
 * @memberof Framebuffer
 */
static SDL_GPUColorTargetInfo colorTargetInfo(const Framebuffer *self, Uint32 index, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp, const SDL_FColor *clearColor) {

  assert(index < self->numColorTargets);
  assert(self->colorTextures[index]);

  SDL_GPUColorTargetInfo info = {
    .texture = self->colorTextures[index]->texture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_color = clearColor ? *clearColor : (SDL_FColor) { 0.f, 0.f, 0.f, 1.f },
  };

  if (self->sampleCount > SDL_GPU_SAMPLECOUNT_1) {
    assert(self->resolveTextures[index]);

    // Resolve the multisampled color into the single-sample resolve target. STORE is
    // promoted to RESOLVE_AND_STORE so the multisampled contents survive for any later
    // load-op pass (e.g. UI drawn over a 3D scene) while keeping the resolve current.
    info.resolve_texture = self->resolveTextures[index]->texture;
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

  assert(info->numColorTargets <= GPU_MAX_COLOR_TARGETS);

  self = (Framebuffer *) super(Object, self, init);
  if (self) {
    self->device = retain(device);

    self->numColorTargets = info->numColorTargets;
    for (Uint32 i = 0; i < info->numColorTargets; i++) {
      self->colorFormats[i] = info->colorFormats[i];
    }
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

  for (Uint32 i = 0; i < self->numColorTargets; i++) {
    release(self->colorTextures[i]);
    release(self->resolveTextures[i]);
    self->colorTextures[i] = NULL;
    self->resolveTextures[i] = NULL;
  }
  release(self->depthTexture);
  self->depthTexture = NULL;
  release(self->resolveDepthTexture);
  self->resolveDepthTexture = NULL;

  self->size = *size;

  const bool multisampled = self->sampleCount > SDL_GPU_SAMPLECOUNT_1;

  for (Uint32 i = 0; i < self->numColorTargets; i++) {

    self->colorTextures[i] = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = self->colorFormats[i],
      .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | (multisampled ? 0 : SDL_GPU_TEXTUREUSAGE_SAMPLER),
      .width = (Uint32) self->size.w,
      .height = (Uint32) self->size.h,
      .layer_count_or_depth = 1,
      .num_levels = 1,
      .sample_count = self->sampleCount,
    }, NULL);

    if (multisampled) {
      self->resolveTextures[i] = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = self->colorFormats[i],
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

    // Single-sample depth carries SAMPLER so it can be read directly via
    // resolveDepthTexture (e.g. soft particles). Multisampled depth is a plain
    // depth-stencil target; sampling it requires the separate resolveDepthTexture,
    // populated by a resolve pass (SDL has no depth store-op resolve) — TODO when MSAA lands.
    self->depthTexture = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = self->depthFormat,
      .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET | (multisampled ? 0 : SDL_GPU_TEXTUREUSAGE_SAMPLER),
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
 * @fn Texture *Framebuffer::resolveColorTexture(const Framebuffer *self, Uint32 index)
 * @memberof Framebuffer
 */
static Texture *resolveColorTexture(const Framebuffer *self, Uint32 index) {
  assert(index < self->numColorTargets);
  return self->resolveTextures[index] ?: self->colorTextures[index];
}

/**
 * @fn Texture *Framebuffer::resolveDepthTexture(const Framebuffer *self)
 * @memberof Framebuffer
 */
static Texture *resolveDepthTexture(const Framebuffer *self) {
  return self->resolveDepthTexture ?: self->depthTexture;
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
  ((FramebufferInterface *) clazz->interface)->resolveColorTexture = resolveColorTexture;
  ((FramebufferInterface *) clazz->interface)->resolveDepthTexture = resolveDepthTexture;
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
