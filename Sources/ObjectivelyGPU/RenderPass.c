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

#include "CommandBuffer.h"
#include "GraphicsPipeline.h"
#include "RenderPass.h"

#define _Class _RenderPass

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  RenderPass *this = (RenderPass *) self;
  if (this->pass) {
    SDL_EndGPURenderPass(this->pass);
  }

  super(Object, self, dealloc);
}

#pragma mark - RenderPass

/**
 * @fn void RenderPass::bindFragmentSamplers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num)
 * @memberof RenderPass
 */
static void bindFragmentSamplers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num) {
  SDL_BindGPUFragmentSamplers(self->pass, firstSlot, bindings, num);
}

/**
 * @fn void RenderPass::bindFragmentStorageBuffers(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num)
 * @memberof RenderPass
 */
static void bindFragmentStorageBuffers(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num) {
  SDL_BindGPUFragmentStorageBuffers(self->pass, firstSlot, buffers, num);
}

/**
 * @fn void RenderPass::bindFragmentStorageTextures(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num)
 * @memberof RenderPass
 */
static void bindFragmentStorageTextures(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num) {
  SDL_BindGPUFragmentStorageTextures(self->pass, firstSlot, textures, num);
}

/**
 * @fn void RenderPass::bindIndexBuffer(const RenderPass *self, const SDL_GPUBufferBinding *binding, SDL_GPUIndexElementSize indexElementSize)
 * @memberof RenderPass
 */
static void bindIndexBuffer(const RenderPass *self, const SDL_GPUBufferBinding *binding, SDL_GPUIndexElementSize indexElementSize) {
  SDL_BindGPUIndexBuffer(self->pass, binding, indexElementSize);
}

/**
 * @fn void RenderPass::bindPipeline(RenderPass *self, GraphicsPipeline *pipeline)
 * @memberof RenderPass
 */
static void bindPipeline(RenderPass *self, GraphicsPipeline *pipeline) {

  assert(pipeline);

  self->pipeline = pipeline;

  SDL_BindGPUGraphicsPipeline(self->pass, pipeline->pipeline);
}

/**
 * @fn void RenderPass::bindVertexBuffers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUBufferBinding *bindings, Uint32 num)
 * @memberof RenderPass
 */
static void bindVertexBuffers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUBufferBinding *bindings, Uint32 num) {
  SDL_BindGPUVertexBuffers(self->pass, firstSlot, bindings, num);
}

/**
 * @fn void RenderPass::bindVertexSamplers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num)
 * @memberof RenderPass
 */
static void bindVertexSamplers(const RenderPass *self, Uint32 firstSlot, const SDL_GPUTextureSamplerBinding *bindings, Uint32 num) {
  SDL_BindGPUVertexSamplers(self->pass, firstSlot, bindings, num);
}

/**
 * @fn void RenderPass::bindVertexStorageBuffers(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num)
 * @memberof RenderPass
 */
static void bindVertexStorageBuffers(const RenderPass *self, Uint32 firstSlot, SDL_GPUBuffer *const *buffers, Uint32 num) {
  SDL_BindGPUVertexStorageBuffers(self->pass, firstSlot, buffers, num);
}

/**
 * @fn void RenderPass::bindVertexStorageTextures(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num)
 * @memberof RenderPass
 */
static void bindVertexStorageTextures(const RenderPass *self, Uint32 firstSlot, SDL_GPUTexture *const *textures, Uint32 num) {
  SDL_BindGPUVertexStorageTextures(self->pass, firstSlot, textures, num);
}

/**
 * @fn void RenderPass::drawIndexedPrimitives(const RenderPass *self, Uint32 numIndices, Uint32 numInstances, Uint32 firstIndex, Sint32 vertexOffset, Uint32 firstInstance)
 * @memberof RenderPass
 */
static void drawIndexedPrimitives(const RenderPass *self, Uint32 numIndices, Uint32 numInstances, Uint32 firstIndex, Sint32 vertexOffset, Uint32 firstInstance) {
  GPU_Assert(self->pipeline, "No pipeline bound");
  SDL_DrawGPUIndexedPrimitives(self->pass, numIndices, numInstances, firstIndex, vertexOffset, firstInstance);
}

/**
 * @fn void RenderPass::drawIndexedPrimitivesIndirect(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount)
 * @memberof RenderPass
 */
static void drawIndexedPrimitivesIndirect(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount) {
  GPU_Assert(self->pipeline, "No pipeline bound");
  SDL_DrawGPUIndexedPrimitivesIndirect(self->pass, buffer, offset, drawCount);
}

/**
 * @fn void RenderPass::drawPrimitives(const RenderPass *self, Uint32 numVertices, Uint32 numInstances, Uint32 firstVertex, Uint32 firstInstance)
 * @memberof RenderPass
 */
static void drawPrimitives(const RenderPass *self, Uint32 numVertices, Uint32 numInstances, Uint32 firstVertex, Uint32 firstInstance) {
  GPU_Assert(self->pipeline, "No pipeline bound");
  SDL_DrawGPUPrimitives(self->pass, numVertices, numInstances, firstVertex, firstInstance);
}

/**
 * @fn void RenderPass::drawPrimitivesIndirect(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount)
 * @memberof RenderPass
 */
static void drawPrimitivesIndirect(const RenderPass *self, SDL_GPUBuffer *buffer, Uint32 offset, Uint32 drawCount) {
  GPU_Assert(self->pipeline, "No pipeline bound");
  SDL_DrawGPUPrimitivesIndirect(self->pass, buffer, offset, drawCount);
}

/**
 * @fn RenderPass *RenderPass::init(RenderPass *self, CommandBuffer *commands, SDL_GPURenderPass *pass)
 * @memberof RenderPass
 */
static RenderPass *init(RenderPass *self, CommandBuffer *commands, SDL_GPURenderPass *pass) {

  self = (RenderPass *) super(Object, self, init);
  if (self) {
    self->commands = commands;
    assert(self->commands);

    self->pass = pass;
    assert(self->pass);
  }

  return self;
}

/**
 * @fn void RenderPass::setBlendConstants(RenderPass *self, SDL_FColor blendConstants)
 * @memberof RenderPass
 */
static void setBlendConstants(RenderPass *self, SDL_FColor blendConstants) {

  self->blendConstants = blendConstants;

  SDL_SetGPUBlendConstants(self->pass, self->blendConstants);
}

/**
 * @fn void RenderPass::setScissor(const RenderPass *self, const SDL_Rect *scissor)
 * @memberof RenderPass
 */
static void setScissor(RenderPass *self, const SDL_Rect *scissor) {

  if (scissor) {
    self->scissor = *scissor;
  } else {
    // Reset to the full render target. RenderPass does not yet capture the
    // target dimensions, so fall back to the current viewport, which the
    // caller is expected to have set. This becomes a true target-size reset
    // once Texture objects carry their dimensions into beginRenderPass.
    self->scissor = (SDL_Rect) {
      (int) self->viewport.x,
      (int) self->viewport.y,
      (int) self->viewport.w,
      (int) self->viewport.h,
    };
  }

  SDL_SetGPUScissor(self->pass, &self->scissor);
}

/**
 * @fn void RenderPass::setStencilReference(RenderPass *self, Uint8 reference)
 * @memberof RenderPass
 */
static void setStencilReference(RenderPass *self, Uint8 reference) {

  self->stencilReference = reference;

  SDL_SetGPUStencilReference(self->pass, self->stencilReference);
}

/**
 * @fn void RenderPass::setViewport(const RenderPass *self, const SDL_GPUViewport *viewport)
 * @memberof RenderPass
 */
static void setViewport(RenderPass *self, const SDL_GPUViewport *viewport) {

  assert(viewport);

  self->viewport = *viewport;

  SDL_SetGPUViewport(self->pass, &self->viewport);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((RenderPassInterface *) clazz->interface)->bindFragmentSamplers = bindFragmentSamplers;
  ((RenderPassInterface *) clazz->interface)->bindFragmentStorageBuffers = bindFragmentStorageBuffers;
  ((RenderPassInterface *) clazz->interface)->bindFragmentStorageTextures = bindFragmentStorageTextures;
  ((RenderPassInterface *) clazz->interface)->bindIndexBuffer = bindIndexBuffer;
  ((RenderPassInterface *) clazz->interface)->bindPipeline = bindPipeline;
  ((RenderPassInterface *) clazz->interface)->bindVertexBuffers = bindVertexBuffers;
  ((RenderPassInterface *) clazz->interface)->bindVertexSamplers = bindVertexSamplers;
  ((RenderPassInterface *) clazz->interface)->bindVertexStorageBuffers = bindVertexStorageBuffers;
  ((RenderPassInterface *) clazz->interface)->bindVertexStorageTextures = bindVertexStorageTextures;
  ((RenderPassInterface *) clazz->interface)->drawIndexedPrimitives = drawIndexedPrimitives;
  ((RenderPassInterface *) clazz->interface)->drawIndexedPrimitivesIndirect = drawIndexedPrimitivesIndirect;
  ((RenderPassInterface *) clazz->interface)->drawPrimitives = drawPrimitives;
  ((RenderPassInterface *) clazz->interface)->drawPrimitivesIndirect = drawPrimitivesIndirect;
  ((RenderPassInterface *) clazz->interface)->init = init;
  ((RenderPassInterface *) clazz->interface)->setBlendConstants = setBlendConstants;
  ((RenderPassInterface *) clazz->interface)->setScissor = setScissor;
  ((RenderPassInterface *) clazz->interface)->setStencilReference = setStencilReference;
  ((RenderPassInterface *) clazz->interface)->setViewport = setViewport;
}

/**
 * @fn Class *RenderPass::_RenderPass(void)
 * @memberof RenderPass
 */
Class *_RenderPass(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "RenderPass",
      .superclass      = _Object(),
      .instanceSize    = sizeof(RenderPass),
      .interfaceOffset = offsetof(RenderPass, interface),
      .interfaceSize   = sizeof(RenderPassInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
