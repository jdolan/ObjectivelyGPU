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

  for (Uint32 i = 0; i < this->numColorAttachments; i++) {
    release(this->colorAttachments[i].textures[0]);
    release(this->colorAttachments[i].textures[1]);
    release(this->colorAttachments[i].resolveTextures[0]);
    release(this->colorAttachments[i].resolveTextures[1]);
  }
  release(this->depthAttachment.textures[0]);
  release(this->depthAttachment.resolveTextures[0]);

  release(this->device);

  super(Object, self, dealloc);
}

#pragma mark - Framebuffer

/**
 * @brief Returns the physical slot index a double-buffered attachment is writing this frame.
 */
static Uint32 currentSlot(const Framebuffer *self, const GPU_FramebufferAttachment *attachment) {
  return attachment->doubleBuffered ? (self->frame & 1) : 0;
}

/**
 * @fn SDL_GPUColorTargetInfo Framebuffer::colorTargetInfo(const Framebuffer *self, Uint32 index, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp)
 * @memberof Framebuffer
 */
static SDL_GPUColorTargetInfo colorTargetInfo(const Framebuffer *self, Uint32 index, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp) {

  assert(index < self->numColorAttachments);

  const GPU_FramebufferAttachment *attachment = &self->colorAttachments[index];
  const Uint32 slot = currentSlot(self, attachment);

  assert(attachment->textures[slot]);

  SDL_GPUColorTargetInfo info = {
    .texture = attachment->textures[slot]->texture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_color = attachment->clearColor,
  };

  if (self->sampleCount > SDL_GPU_SAMPLECOUNT_1) {
    assert(attachment->resolveTextures[slot]);

    // Resolve the multisampled color into the single-sample resolve target. STORE is
    // promoted to RESOLVE_AND_STORE so the multisampled contents survive for any later
    // load-op pass (e.g. UI drawn over a 3D scene) while keeping the resolve current.
    info.resolve_texture = attachment->resolveTextures[slot]->texture;
    if (storeOp == SDL_GPU_STOREOP_STORE) {
      info.store_op = SDL_GPU_STOREOP_RESOLVE_AND_STORE;
    }
  }

  return info;
}

/**
 * @fn SDL_GPUDepthStencilTargetInfo Framebuffer::depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp)
 * @memberof Framebuffer
 */
static SDL_GPUDepthStencilTargetInfo depthTargetInfo(const Framebuffer *self, SDL_GPULoadOp loadOp, SDL_GPUStoreOp storeOp) {

  assert(self->depthAttachment.textures[0]);

  return (SDL_GPUDepthStencilTargetInfo) {
    .texture = self->depthAttachment.textures[0]->texture,
    .load_op = loadOp,
    .store_op = storeOp,
    .clear_depth = self->depthAttachment.clearDepth,
  };
}

/**
 * @fn void Framebuffer::pipelineTargetInfo(const Framebuffer *self, const SDL_GPUColorTargetBlendState *blendStates, SDL_GPUColorTargetDescription *descriptions, SDL_GPUGraphicsPipelineTargetInfo *targetInfo)
 * @memberof Framebuffer
 */
static void pipelineTargetInfo(const Framebuffer *self, const SDL_GPUColorTargetBlendState *blendStates,
                               SDL_GPUColorTargetDescription *descriptions, SDL_GPUGraphicsPipelineTargetInfo *targetInfo) {

  for (Uint32 i = 0; i < self->numColorAttachments; i++) {
    descriptions[i] = (SDL_GPUColorTargetDescription) {
      .format = self->colorAttachments[i].format,
      .blend_state = blendStates[i],
    };
  }

  *targetInfo = (SDL_GPUGraphicsPipelineTargetInfo) {
    .color_target_descriptions = descriptions,
    .num_color_targets = self->numColorAttachments,
    .depth_stencil_format = self->depthAttachment.format,
    .has_depth_stencil_target = self->depthAttachment.format != SDL_GPU_TEXTUREFORMAT_INVALID,
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

    self->numColorAttachments = info->numColorTargets;
    for (Uint32 i = 0; i < info->numColorTargets; i++) {
      self->colorAttachments[i].format = info->colorAttachments[i].format;
      self->colorAttachments[i].clearColor = info->colorAttachments[i].clearColor;
      self->colorAttachments[i].doubleBuffered = info->colorAttachments[i].doubleBuffered;
    }
    self->depthAttachment.format = info->depthAttachment.format;
    self->depthAttachment.clearDepth = info->depthAttachment.clearDepth;

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

  for (Uint32 i = 0; i < self->numColorAttachments; i++) {
    GPU_FramebufferAttachment *attachment = &self->colorAttachments[i];
    release(attachment->textures[0]);
    release(attachment->textures[1]);
    release(attachment->resolveTextures[0]);
    release(attachment->resolveTextures[1]);
    attachment->textures[0] = attachment->textures[1] = NULL;
    attachment->resolveTextures[0] = attachment->resolveTextures[1] = NULL;
  }
  release(self->depthAttachment.textures[0]);
  self->depthAttachment.textures[0] = NULL;
  release(self->depthAttachment.resolveTextures[0]);
  self->depthAttachment.resolveTextures[0] = NULL;

  self->size = *size;
  self->frame = 0;

  const bool multisampled = self->sampleCount > SDL_GPU_SAMPLECOUNT_1;

  for (Uint32 i = 0; i < self->numColorAttachments; i++) {

    GPU_FramebufferAttachment *attachment = &self->colorAttachments[i];
    const Uint32 slots = attachment->doubleBuffered ? 2 : 1;

    for (Uint32 slot = 0; slot < slots; slot++) {

      attachment->textures[slot] = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = attachment->format,
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | (multisampled ? 0 : SDL_GPU_TEXTUREUSAGE_SAMPLER),
        .width = (Uint32) self->size.w,
        .height = (Uint32) self->size.h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = self->sampleCount,
      }, NULL);

      if (multisampled) {
        attachment->resolveTextures[slot] = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
          .type = SDL_GPU_TEXTURETYPE_2D,
          .format = attachment->format,
          .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
          .width = (Uint32) self->size.w,
          .height = (Uint32) self->size.h,
          .layer_count_or_depth = 1,
          .num_levels = 1,
          .sample_count = SDL_GPU_SAMPLECOUNT_1,
        }, NULL);
      }
    }
  }

  if (self->depthAttachment.format != SDL_GPU_TEXTUREFORMAT_INVALID) {

    // Single-sample depth carries SAMPLER so it can be read directly via
    // resolveDepthTexture (e.g. soft particles). Multisampled depth is a plain
    // depth-stencil target; sampling it requires the separate resolve texture,
    // populated by a resolve pass (SDL has no depth store-op resolve) — TODO when MSAA lands.
    self->depthAttachment.textures[0] = $(self->device, createTexture, &(SDL_GPUTextureCreateInfo) {
      .type = SDL_GPU_TEXTURETYPE_2D,
      .format = self->depthAttachment.format,
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
  assert(index < self->numColorAttachments);
  const GPU_FramebufferAttachment *attachment = &self->colorAttachments[index];
  const Uint32 slot = currentSlot(self, attachment);
  return attachment->resolveTextures[slot] ?: attachment->textures[slot];
}

/**
 * @fn Texture *Framebuffer::previousColorTexture(const Framebuffer *self, Uint32 index)
 * @memberof Framebuffer
 */
static Texture *previousColorTexture(const Framebuffer *self, Uint32 index) {
  assert(index < self->numColorAttachments);
  const GPU_FramebufferAttachment *attachment = &self->colorAttachments[index];
  assert(attachment->doubleBuffered);
  const Uint32 slot = currentSlot(self, attachment) ^ 1;
  return attachment->resolveTextures[slot] ?: attachment->textures[slot];
}

/**
 * @fn Texture *Framebuffer::resolveDepthTexture(const Framebuffer *self)
 * @memberof Framebuffer
 */
static Texture *resolveDepthTexture(const Framebuffer *self) {
  return self->depthAttachment.resolveTextures[0] ?: self->depthAttachment.textures[0];
}

/**
 * @fn void Framebuffer::swap(Framebuffer *self)
 * @memberof Framebuffer
 */
static void swap(Framebuffer *self) {
  self->frame++;
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
  ((FramebufferInterface *) clazz->interface)->pipelineTargetInfo = pipelineTargetInfo;
  ((FramebufferInterface *) clazz->interface)->previousColorTexture = previousColorTexture;
  ((FramebufferInterface *) clazz->interface)->resize = resize;
  ((FramebufferInterface *) clazz->interface)->resolveColorTexture = resolveColorTexture;
  ((FramebufferInterface *) clazz->interface)->resolveDepthTexture = resolveDepthTexture;
  ((FramebufferInterface *) clazz->interface)->swap = swap;
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
