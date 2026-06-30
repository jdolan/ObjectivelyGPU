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
#include "ComputePass.h"
#include "CopyPass.h"
#include "RenderDevice.h"
#include "RenderPass.h"

#define _Class _CommandBuffer

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  CommandBuffer *this = (CommandBuffer *) self;

  GPU_Assert(!this->pass, "command buffer released with an open pass");
  GPU_Assert(this->submitted, "command buffer released without submit or cancel");

  super(Object, self, dealloc);
}

#pragma mark - CommandBuffer

/**
 * @fn bool CommandBuffer::acquireSwapchainTexture(const CommandBuffer *self, SwapchainTexture *swapchain)
 * @memberof CommandBuffer
 */
static bool acquireSwapchainTexture(const CommandBuffer *self, SwapchainTexture *swapchain) {

  assert(swapchain);

  return SDL_AcquireGPUSwapchainTexture(self->commands,
                                        self->device->window,
                                        &swapchain->texture,
                                        (Uint32 *) &swapchain->size.w,
                                        (Uint32 *) &swapchain->size.h);
}

/**
 * @fn ComputePass *CommandBuffer::beginComputePass(CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers)
 * @memberof CommandBuffer
 */
static ComputePass *beginComputePass(CommandBuffer *self, const SDL_GPUStorageTextureReadWriteBinding *storageTextures, Uint32 numStorageTextures, const SDL_GPUStorageBufferReadWriteBinding *storageBuffers, Uint32 numStorageBuffers) {

  GPU_Assert(!self->submitted, "command buffer already submitted");
  GPU_Assert(!self->pass, "a pass is already open");

  SDL_GPUComputePass *pass = SDL_BeginGPUComputePass(self->commands, storageTextures, numStorageTextures, storageBuffers, numStorageBuffers);
  GPU_Assert(pass, "SDL_BeginGPUComputePass");

  ComputePass *computePass = $(alloc(ComputePass), init, self, pass);
  self->pass = (Object *) computePass;

  return computePass;
}

/**
 * @fn CopyPass *CommandBuffer::beginCopyPass(CommandBuffer *self)
 * @memberof CommandBuffer
 */
static CopyPass *beginCopyPass(CommandBuffer *self) {

  GPU_Assert(!self->submitted, "command buffer already submitted");
  GPU_Assert(!self->pass, "a pass is already open");

  SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(self->commands);
  GPU_Assert(pass, "SDL_BeginGPUCopyPass");

  CopyPass *copyPass = $(alloc(CopyPass), init, self, pass);
  self->pass = (Object *) copyPass;

  return copyPass;
}

/**
 * @fn RenderPass *CommandBuffer::beginRenderPass(CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil)
 * @memberof CommandBuffer
 */
static RenderPass *beginRenderPass(CommandBuffer *self, const SDL_GPUColorTargetInfo *colorTargets, Uint32 numColorTargets, const SDL_GPUDepthStencilTargetInfo *depthStencil) {

  GPU_Assert(!self->submitted, "command buffer already submitted");
  GPU_Assert(!self->pass, "a pass is already open");

  SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(self->commands, colorTargets, numColorTargets, depthStencil);
  GPU_Assert(pass, "SDL_BeginGPURenderPass");

  RenderPass *renderPass = $(alloc(RenderPass), init, self, pass);
  self->pass = (Object *) renderPass;

  return renderPass;
}

/**
 * @fn void CommandBuffer::blitTexture(const CommandBuffer *self, const SDL_GPUBlitInfo *info)
 * @memberof CommandBuffer
 */
static void blitTexture(const CommandBuffer *self, const SDL_GPUBlitInfo *info) {
  SDL_BlitGPUTexture(self->commands, info);
}

/**
 * @fn bool CommandBuffer::cancel(CommandBuffer *self)
 * @memberof CommandBuffer
 */
static bool cancel(CommandBuffer *self) {

  GPU_Assert(!self->submitted, "command buffer already submitted");
  GPU_Assert(!self->pass, "cannot cancel with an open pass");

  const bool ok = SDL_CancelGPUCommandBuffer(self->commands);
  GPU_Assert(ok, "SDL_CancelGPUCommandBuffer");

  self->submitted = true;

  return ok;
}

/**
 * @fn void CommandBuffer::generateMipmaps(const CommandBuffer *self, SDL_GPUTexture *texture)
 * @memberof CommandBuffer
 */
static void generateMipmaps(const CommandBuffer *self, SDL_GPUTexture *texture) {


  SDL_GenerateMipmapsForGPUTexture(self->commands, texture);
}

/**
 * @fn CommandBuffer *CommandBuffer::initWithCommandBuffer(CommandBuffer *self, const RenderDevice *device, SDL_GPUCommandBuffer *commands)
 * @memberof CommandBuffer
 */
static CommandBuffer *initWithCommandBuffer(CommandBuffer *self, const RenderDevice *device, SDL_GPUCommandBuffer *commands) {

  self = (CommandBuffer *) super(Object, self, init);
  if (self) {
    assert(device);
    self->device = (RenderDevice *) device;
    self->commands = commands;
    assert(self->commands);
  }

  return self;
}

/**
 * @fn void CommandBuffer::insertDebugLabel(const CommandBuffer *self, const char *text)
 * @memberof CommandBuffer
 */
static void insertDebugLabel(const CommandBuffer *self, const char *text) {
  SDL_InsertGPUDebugLabel(self->commands, text);
}

/**
 * @fn void CommandBuffer::popDebugGroup(const CommandBuffer *self)
 * @memberof CommandBuffer
 */
static void popDebugGroup(const CommandBuffer *self) {
  SDL_PopGPUDebugGroup(self->commands);
}

/**
 * @fn void CommandBuffer::pushComputeUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
 * @memberof CommandBuffer
 */
static void pushComputeUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length) {
  SDL_PushGPUComputeUniformData(self->commands, slot, data, length);
}

/**
 * @fn void CommandBuffer::pushDebugGroup(const CommandBuffer *self, const char *name)
 * @memberof CommandBuffer
 */
static void pushDebugGroup(const CommandBuffer *self, const char *name) {
  SDL_PushGPUDebugGroup(self->commands, name);
}

/**
 * @fn void CommandBuffer::pushFragmentUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
 * @memberof CommandBuffer
 */
static void pushFragmentUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length) {
  SDL_PushGPUFragmentUniformData(self->commands, slot, data, length);
}

/**
 * @fn void CommandBuffer::pushVertexUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length)
 * @memberof CommandBuffer
 */
static void pushVertexUniformData(const CommandBuffer *self, Uint32 slot, const void *data, Uint32 length) {
  SDL_PushGPUVertexUniformData(self->commands, slot, data, length);
}

/**
 * @fn bool CommandBuffer::submit(CommandBuffer *self)
 * @memberof CommandBuffer
 */
static bool submit(CommandBuffer *self) {

  GPU_Assert(!self->submitted, "command buffer already submitted");
  GPU_Assert(!self->pass, "cannot submit with an open pass");

  const bool ok = SDL_SubmitGPUCommandBuffer(self->commands);
  GPU_Assert(ok, "SDL_SubmitGPUCommandBuffer");

  self->submitted = true;

  return ok;
}

/**
 * @fn SDL_GPUFence *CommandBuffer::submitAndFence(CommandBuffer *self)
 * @memberof CommandBuffer
 */
static SDL_GPUFence *submitAndFence(CommandBuffer *self) {

  GPU_Assert(!self->submitted, "command buffer already submitted");
  GPU_Assert(!self->pass, "cannot submit with an open pass");

  SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(self->commands);
  GPU_Assert(fence, "SDL_SubmitGPUCommandBufferAndAcquireFence");

  self->submitted = true;

  return fence;
}

/**
 * @fn bool CommandBuffer::waitAndAcquireSwapchainTexture(const CommandBuffer *self, SwapchainTexture *swapchain)
 * @memberof CommandBuffer
 */
static bool waitAndAcquireSwapchainTexture(const CommandBuffer *self, SwapchainTexture *swapchain) {

  assert(swapchain);

  return SDL_WaitAndAcquireGPUSwapchainTexture(self->commands,
                                               self->device->window,
                                               &swapchain->texture,
                                               (Uint32 *) &swapchain->size.w,
                                               (Uint32 *) &swapchain->size.h);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

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
