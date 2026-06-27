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
#include "CopyPass.h"
#include "RenderPass.h"

#define _Class _CommandBuffer

#pragma mark - CommandBuffer

/**
 * @fn bool CommandBuffer::acquireSwapchainTexture(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h)
 * @memberof CommandBuffer
 */
static bool acquireSwapchainTexture(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h) {
  return SDL_AcquireGPUSwapchainTexture(self->cmd, window, texture, w, h);
}

/**
 * @fn ComputePass *CommandBuffer::beginComputePass(const CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers)
 * @memberof CommandBuffer
 */
static ComputePass *beginComputePass(const CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers) {

  SDL_GPUComputePass *pass = SDL_BeginGPUComputePass(self->cmd, storageTextures, numStorageTextures, storageBuffers, numStorageBuffers);
  GPU_Assert(pass, "SDL_BeginGPUComputePass");

  return $(alloc(ComputePass), init, pass);
}

/**
 * @fn CopyPass *CommandBuffer::beginCopyPass(const CommandBuffer *self)
 * @memberof CommandBuffer
 */
static CopyPass *beginCopyPass(const CommandBuffer *self) {

  SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(self->cmd);
  GPU_Assert(pass, "SDL_BeginGPUCopyPass");

  return $(alloc(CopyPass), init, pass, self->device);
}

/**
 * @fn RenderPass *CommandBuffer::beginRenderPass(const CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil)
 * @memberof CommandBuffer
 */
static RenderPass *beginRenderPass(const CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil) {

  SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(self->cmd, colorTargets, numColorTargets, depthStencil);
  GPU_Assert(pass, "SDL_BeginGPURenderPass");

  return $(alloc(RenderPass), init, pass);
}

/**
 * @fn void CommandBuffer::blitTexture(const CommandBuffer *self, const SDL_GPUBlitInfo *info)
 * @memberof CommandBuffer
 */
static void blitTexture(const CommandBuffer *self, const SDL_GPUBlitInfo *info) {
  SDL_BlitGPUTexture(self->cmd, info);
}

/**
 * @fn bool CommandBuffer::cancel(const CommandBuffer *self)
 * @memberof CommandBuffer
 */
static bool cancel(const CommandBuffer *self) {

  const bool ok = SDL_CancelGPUCommandBuffer(self->cmd);
  GPU_Assert(ok, "SDL_CancelGPUCommandBuffer");

  return ok;
}

/**
 * @fn void CommandBuffer::generateMipmaps(const CommandBuffer *self, SDL_GPUTexture *texture)
 * @memberof CommandBuffer
 */
static void generateMipmaps(const CommandBuffer *self, SDL_GPUTexture *texture) {


  SDL_GenerateMipmapsForGPUTexture(self->cmd, texture);
}

/**
 * @fn CommandBuffer *CommandBuffer::initWithCommandBuffer(CommandBuffer *self, SDL_GPUCommandBuffer *cmd)
 * @memberof CommandBuffer
 */
static CommandBuffer *initWithCommandBuffer(CommandBuffer *self, SDL_GPUCommandBuffer *cmd) {

  self = (CommandBuffer *) super(Object, self, init);
  if (self) {
    self->cmd = cmd;
    assert(self->cmd);
  }

  return self;
}

/**
 * @fn void CommandBuffer::insertDebugLabel(const CommandBuffer *self, const char *text)
 * @memberof CommandBuffer
 */
static void insertDebugLabel(const CommandBuffer *self, const char *text) {
  SDL_InsertGPUDebugLabel(self->cmd, text);
}

/**
 * @fn void CommandBuffer::popDebugGroup(const CommandBuffer *self)
 * @memberof CommandBuffer
 */
static void popDebugGroup(const CommandBuffer *self) {
  SDL_PopGPUDebugGroup(self->cmd);
}

/**
 * @fn void CommandBuffer::pushComputeUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
 * @memberof CommandBuffer
 */
static void pushComputeUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length) {
  SDL_PushGPUComputeUniformData(self->cmd, slot, data, length);
}

/**
 * @fn void CommandBuffer::pushDebugGroup(const CommandBuffer *self, const char *name)
 * @memberof CommandBuffer
 */
static void pushDebugGroup(const CommandBuffer *self, const char *name) {
  SDL_PushGPUDebugGroup(self->cmd, name);
}

/**
 * @fn void CommandBuffer::pushFragmentUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
 * @memberof CommandBuffer
 */
static void pushFragmentUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length) {
  SDL_PushGPUFragmentUniformData(self->cmd, slot, data, length);
}

/**
 * @fn void CommandBuffer::pushVertexUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
 * @memberof CommandBuffer
 */
static void pushVertexUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length) {
  SDL_PushGPUVertexUniformData(self->cmd, slot, data, length);
}

/**
 * @fn bool CommandBuffer::submit(const CommandBuffer *self)
 * @memberof CommandBuffer
 */
static bool submit(const CommandBuffer *self) {

  const bool ok = SDL_SubmitGPUCommandBuffer(self->cmd);
  GPU_Assert(ok, "SDL_SubmitGPUCommandBuffer");

  return ok;
}

/**
 * @fn SDL_GPUFence *CommandBuffer::submitAndFence(const CommandBuffer *self)
 * @memberof CommandBuffer
 */
static SDL_GPUFence *submitAndFence(const CommandBuffer *self) {

  SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(self->cmd);
  GPU_Assert(fence, "SDL_SubmitGPUCommandBufferAndAcquireFence");

  return fence;
}

/**
 * @fn bool CommandBuffer::waitAndAcquireSwapchainTexture(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h)
 * @memberof CommandBuffer
 */
static bool waitAndAcquireSwapchainTexture(const CommandBuffer *self, SDL_Window *window, SDL_GPUTexture **texture, Uint32 *w, Uint32 *h) {

  const bool ok = SDL_WaitAndAcquireGPUSwapchainTexture(self->cmd, window, texture, w, h);
  GPU_Assert(ok, "SDL_WaitAndAcquireGPUSwapchainTexture");

  return ok;
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((CommandBufferInterface *) clazz->interface)->acquireSwapchainTexture = acquireSwapchainTexture;
  ((CommandBufferInterface *) clazz->interface)->beginComputePass = beginComputePass;
  ((CommandBufferInterface *) clazz->interface)->beginCopyPass = beginCopyPass;
  ((CommandBufferInterface *) clazz->interface)->beginRenderPass = beginRenderPass;
  ((CommandBufferInterface *) clazz->interface)->blitTexture = blitTexture;
  ((CommandBufferInterface *) clazz->interface)->cancel = cancel;
  ((CommandBufferInterface *) clazz->interface)->generateMipmaps = generateMipmaps;
  ((CommandBufferInterface *) clazz->interface)->initWithCommandBuffer = initWithCommandBuffer;
  ((CommandBufferInterface *) clazz->interface)->insertDebugLabel = insertDebugLabel;
  ((CommandBufferInterface *) clazz->interface)->popDebugGroup = popDebugGroup;
  ((CommandBufferInterface *) clazz->interface)->pushComputeUniformData = pushComputeUniformData;
  ((CommandBufferInterface *) clazz->interface)->pushDebugGroup = pushDebugGroup;
  ((CommandBufferInterface *) clazz->interface)->pushFragmentUniformData = pushFragmentUniformData;
  ((CommandBufferInterface *) clazz->interface)->pushVertexUniformData = pushVertexUniformData;
  ((CommandBufferInterface *) clazz->interface)->submit = submit;
  ((CommandBufferInterface *) clazz->interface)->submitAndFence = submitAndFence;
  ((CommandBufferInterface *) clazz->interface)->waitAndAcquireSwapchainTexture = waitAndAcquireSwapchainTexture;
}

/**
 * @fn Class *CommandBuffer::_CommandBuffer(void)
 * @memberof CommandBuffer
 */
Class *_CommandBuffer(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "CommandBuffer",
      .superclass      = _Object(),
      .instanceSize    = sizeof(CommandBuffer),
      .interfaceOffset = offsetof(CommandBuffer, interface),
      .interfaceSize   = sizeof(CommandBufferInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
