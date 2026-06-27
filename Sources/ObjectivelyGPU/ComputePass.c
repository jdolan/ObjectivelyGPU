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

#include "CommandBuffer.h"
#include "ComputePass.h"

#define _Class _ComputePass

#pragma mark - Object lifecycle

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  ComputePass *this = (ComputePass *) self;
  if (this->pass) {
    SDL_EndGPUComputePass(this->pass);
  }

  super(Object, self, dealloc);
}

#pragma mark - ComputePass

/**
 * @fn void ComputePass::bindPipeline(const ComputePass *self, SDL_GPUComputePipeline *pipeline)
 * @memberof ComputePass
 */
static void bindPipeline(const ComputePass *self, SDL_GPUComputePipeline *pipeline) {
  SDL_BindGPUComputePipeline(self->pass, pipeline);
}

/**
 * @fn void ComputePass::bindSamplers(const ComputePass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num)
 * @memberof ComputePass
 */
static void bindSamplers(const ComputePass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num) {
  SDL_BindGPUComputeSamplers(self->pass, firstSlot, bindings, num);
}

/**
 * @fn void ComputePass::bindStorageBuffers(const ComputePass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num)
 * @memberof ComputePass
 */
static void bindStorageBuffers(const ComputePass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num) {
  SDL_BindGPUComputeStorageBuffers(self->pass, firstSlot, buffers, num);
}

/**
 * @fn void ComputePass::bindStorageTextures(const ComputePass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num)
 * @memberof ComputePass
 */
static void bindStorageTextures(const ComputePass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num) {
  SDL_BindGPUComputeStorageTextures(self->pass, firstSlot, textures, num);
}

/**
 * @fn void ComputePass::dispatchCompute(const ComputePass *self, Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ)
 * @memberof ComputePass
 */
static void dispatchCompute(const ComputePass *self, Uint32 groupCountX, Uint32 groupCountY, Uint32 groupCountZ) {
  SDL_DispatchGPUCompute(self->pass, groupCountX, groupCountY, groupCountZ);
}

/**
 * @fn void ComputePass::dispatchComputeIndirect(const ComputePass *self, SDL_GPUBuffer *buffer, Uint32 offset)
 * @memberof ComputePass
 */
static void dispatchComputeIndirect(const ComputePass *self, SDL_GPUBuffer *buffer, Uint32 offset) {
  SDL_DispatchGPUComputeIndirect(self->pass, buffer, offset);
}

/**
 * @fn ComputePass *ComputePass::init(ComputePass *self, SDL_GPUComputePass *pass, CommandBuffer *cmd)
 * @memberof ComputePass
 */
static ComputePass *init(ComputePass *self, SDL_GPUComputePass *pass, CommandBuffer *cmd) {

  self = (ComputePass *) super(Object, self, init);
  if (self) {
    self->pass = pass;
    assert(self->pass);
    self->cmd = cmd;
    assert(self->cmd);
  }

  return self;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((ComputePassInterface *) clazz->interface)->bindPipeline = bindPipeline;
  ((ComputePassInterface *) clazz->interface)->bindSamplers = bindSamplers;
  ((ComputePassInterface *) clazz->interface)->bindStorageBuffers = bindStorageBuffers;
  ((ComputePassInterface *) clazz->interface)->bindStorageTextures = bindStorageTextures;
  ((ComputePassInterface *) clazz->interface)->dispatchCompute = dispatchCompute;
  ((ComputePassInterface *) clazz->interface)->dispatchComputeIndirect = dispatchComputeIndirect;
  ((ComputePassInterface *) clazz->interface)->init = init;
}

/**
 * @fn Class *ComputePass::_ComputePass(void)
 * @memberof ComputePass
 */
Class *_ComputePass(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "ComputePass",
      .superclass      = _Object(),
      .instanceSize    = sizeof(ComputePass),
      .interfaceOffset = offsetof(ComputePass, interface),
      .interfaceSize   = sizeof(ComputePassInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
