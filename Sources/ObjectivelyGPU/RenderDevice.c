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

#include <SDL3/SDL_surface.h>

#include <Objectively/Data.h>
#include <Objectively/Resource.h>

#include "Buffer.h"
#include "CommandBuffer.h"
#include "ComputePipeline.h"
#include "CopyPass.h"
#include "Framebuffer.h"
#include "GraphicsPipeline.h"
#include "RenderDevice.h"
#include "Sampler.h"
#include "Shader.h"
#include "Texture.h"

/**
 * @brief Whether to create the `SDL_GPUDevice` with backend validation/debug layers.
 * @details Defaults to enabled unless `NDEBUG` is defined (i.e. on for debug builds,
 *   off for release). Override by defining `GPU_DEBUG` to `true` or `false` at build time.
 */
#ifndef GPU_DEBUG
 #ifdef NDEBUG
  #define GPU_DEBUG false
 #else
  #define GPU_DEBUG true
 #endif
#endif

#define _Class _RenderDevice

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  RenderDevice *this = (RenderDevice *) self;

  release(this->framebuffer);

  if (this->window && this->device) {
    SDL_ReleaseWindowFromGPUDevice(this->device, this->window);
  }

  if (this->device) {
    SDL_DestroyGPUDevice(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - RenderDevice

/**
 * @fn CommandBuffer *RenderDevice::acquireCommandBuffer(const RenderDevice *self)
 * @memberof RenderDevice
 */
static CommandBuffer *acquireCommandBuffer(const RenderDevice *self) {

  SDL_GPUCommandBuffer *commands = SDL_AcquireGPUCommandBuffer(self->device);
  GPU_Assert(commands, "SDL_AcquireGPUCommandBuffer");

  return $(alloc(CommandBuffer), initWithCommandBuffer, self, commands);
}

/**
 * @fn CommandBuffer *RenderDevice::beginFrame(RenderDevice *self)
 * @memberof RenderDevice
 */
static CommandBuffer *beginFrame(RenderDevice *self) {

  GPU_Assert(self->framebuffer, "no framebuffer set; call setFramebuffer first");
  GPU_Assert(!self->commandBuffer, "beginFrame called with a frame already in flight");

  CommandBuffer *commands = $(self, acquireCommandBuffer);

  SwapchainTexture swapchain = { 0 };
  $(commands, waitAndAcquireSwapchainTexture, &swapchain);

  if (!swapchain.texture) {
    $(commands, cancel);
    release(commands);
    return NULL;
  }

  $(self->framebuffer, resize, &swapchain.size);

  self->commandBuffer = commands;
  self->swapchain = swapchain;

  return commands;
}

/**
 * @fn void RenderDevice::endFrame(RenderDevice *self)
 * @memberof RenderDevice
 */
static void endFrame(RenderDevice *self) {

  GPU_Assert(self->commandBuffer, "endFrame called without a frame in flight");

  Texture *color = $(self->framebuffer, resolvedColorTexture);
  GPU_Assert(color, "framebuffer has no color attachment to present");

  $(self->commandBuffer, blitTexture, &(SDL_GPUBlitInfo) {
    .source = {
      .texture = color->texture,
      .w = (Uint32) self->swapchain.size.w,
      .h = (Uint32) self->swapchain.size.h,
    },
    .destination = {
      .texture = self->swapchain.texture,
      .w = (Uint32) self->swapchain.size.w,
      .h = (Uint32) self->swapchain.size.h,
    },
    .load_op = SDL_GPU_LOADOP_DONT_CARE,
    .filter = SDL_GPU_FILTER_NEAREST,
  });

  $(self->commandBuffer, submit);

  release(self->commandBuffer);
  self->commandBuffer = NULL;
  self->swapchain = (SwapchainTexture) { 0 };
}

/**
 * @fn Buffer *RenderDevice::createBuffer(RenderDevice *self, const SDL_GPUBufferCreateInfo *info)
 * @memberof RenderDevice
 */
static Buffer *createBuffer(RenderDevice *self, const SDL_GPUBufferCreateInfo *info) {

  return $(alloc(Buffer), initWithDevice, self, info);
}

/**
 * @fn Buffer *RenderDevice::createBufferWithConstMem(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size)
 * @memberof RenderDevice
 */
static Buffer *createBufferWithConstMem(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size) {

  return $(alloc(Buffer), initWithConstMem, self, usage, mem, size);
}

/**
 * @fn Buffer *RenderDevice::createBufferWithData(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const Data *data)
 * @memberof RenderDevice
 */
static Buffer *createBufferWithData(RenderDevice *self, SDL_GPUBufferUsageFlags usage, const Data *data) {

  return $(alloc(Buffer), initWithData, self, usage, data);
}

/**
 * @fn ComputePipeline *RenderDevice::createComputePipeline(RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static ComputePipeline *createComputePipeline(RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info) {

  return $(alloc(ComputePipeline), initWithDevice, self, info);
}

/**
 * @fn Framebuffer *RenderDevice::createFramebuffer(RenderDevice *self, const GPU_FramebufferCreateInfo *info)
 * @memberof RenderDevice
 */
static Framebuffer *createFramebuffer(RenderDevice *self, const GPU_FramebufferCreateInfo *info) {

  return $(alloc(Framebuffer), initWithDevice, self, info);
}

/**
 * @fn GraphicsPipeline *RenderDevice::createGraphicsPipeline(RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static GraphicsPipeline *createGraphicsPipeline(RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info) {

  return $(alloc(GraphicsPipeline), initWithDevice, self, info);
}

/**
 * @fn Sampler *RenderDevice::createSampler(RenderDevice *self, const SDL_GPUSamplerCreateInfo *info)
 * @memberof RenderDevice
 */
static Sampler *createSampler(RenderDevice *self, const SDL_GPUSamplerCreateInfo *info) {

  return $(alloc(Sampler), initWithDevice, self, info);
}

/**
 * @fn Shader *RenderDevice::createShader(RenderDevice *self, const SDL_GPUShaderCreateInfo *info)
 * @memberof RenderDevice
 */
static Shader *createShader(RenderDevice *self, const SDL_GPUShaderCreateInfo *info) {

  return $(alloc(Shader), initWithDevice, self, info);
}

/**
 * @fn Texture *RenderDevice::createTexture(RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels)
 * @memberof RenderDevice
 */
static Texture *createTexture(RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels) {

  return $(alloc(Texture), initWithDevice, self, info, pixels);
}

/**
 * @fn SDL_GPUTransferBuffer *RenderDevice::createTransferBuffer(const RenderDevice *self, const SDL_GPUTransferBufferCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUTransferBuffer *createTransferBuffer(const RenderDevice *self, const SDL_GPUTransferBufferCreateInfo *info) {

  SDL_GPUTransferBuffer *buffer = SDL_CreateGPUTransferBuffer(self->device, info);
  GPU_Assert(buffer, "SDL_CreateGPUTransferBuffer");

  return buffer;
}

/**
 * @fn Texture *RenderDevice::createTextureFromSurface(RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage)
 * @memberof RenderDevice
 */
static Texture *createTextureFromSurface(RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage) {

  return $(alloc(Texture), initWithSurface, self, surface, usage);
}

/**
 * @fn SDL_GPUTextureFormat RenderDevice::getSwapchainTextureFormat(const RenderDevice *self)
 * @memberof RenderDevice
 */
static SDL_GPUTextureFormat getSwapchainTextureFormat(const RenderDevice *self) {
  assert(self->window);
  return SDL_GetGPUSwapchainTextureFormat(self->device, self->window);
}

/**
 * @fn RenderDevice *RenderDevice::init(RenderDevice *self)
 * @memberof RenderDevice
 */
static RenderDevice *init(RenderDevice *self) {

  self = (RenderDevice *) super(Object, self, init);
  if (self) {

    const SDL_GPUShaderFormat formats =
      SDL_GPU_SHADERFORMAT_MSL |
      SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_DXIL;

    self->device = SDL_CreateGPUDevice(formats, GPU_DEBUG, NULL);
    GPU_Assert(self->device, "SDL_CreateGPUDevice");
  }

  return self;
}

/**
 * @fn RenderDevice *RenderDevice::initWithWindow(RenderDevice *self, SDL_Window *window)
 * @memberof RenderDevice
 */
static RenderDevice *initWithWindow(RenderDevice *self, SDL_Window *window) {

  self = $(self, init);
  if (self) {
    $(self, setWindow, window);
  }
  return self;
}

/**
 * @fn Shader *RenderDevice::loadShader(RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info)
 * @memberof RenderDevice
 */
static Shader *loadShader(RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info) {

  return $(alloc(Shader), initWithResource, self, name, info);
}

/**
 * @fn ComputePipeline *RenderDevice::loadComputePipeline(RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static ComputePipeline *loadComputePipeline(RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info) {

  return $(alloc(ComputePipeline), initWithResource, self, name, info);
}

/**
 * @fn void *RenderDevice::mapTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf, bool cycle)
 * @memberof RenderDevice
 */
static void *mapTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf, bool cycle) {

  void *mapped = SDL_MapGPUTransferBuffer(self->device, tbuf, cycle);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  return mapped;
}

/**
 * @fn bool RenderDevice::queryFence(const RenderDevice *self, SDL_GPUFence *fence)
 * @memberof RenderDevice
 */
static bool queryFence(const RenderDevice *self, SDL_GPUFence *fence) {
  return SDL_QueryGPUFence(self->device, fence);
}

/**
 * @fn void RenderDevice::releaseFence(const RenderDevice *self, SDL_GPUFence *fence)
 * @memberof RenderDevice
 */
static void releaseFence(const RenderDevice *self, SDL_GPUFence *fence) {
  SDL_ReleaseGPUFence(self->device, fence);
}

/**
 * @fn void RenderDevice::releaseTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf)
 * @memberof RenderDevice
 */
static void releaseTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf) {
  SDL_ReleaseGPUTransferBuffer(self->device, tbuf);
}

/**
 * @fn bool RenderDevice::setAllowedFramesInFlight(const RenderDevice *self, Uint32 allowed)
 * @memberof RenderDevice
 */
static bool setAllowedFramesInFlight(const RenderDevice *self, Uint32 allowed) {
  return SDL_SetGPUAllowedFramesInFlight(self->device, allowed);
}

/**
 * @fn void RenderDevice::setFramebuffer(RenderDevice *self, Framebuffer *framebuffer)
 * @memberof RenderDevice
 */
static void setFramebuffer(RenderDevice *self, Framebuffer *framebuffer) {

  if (self->framebuffer != framebuffer) {
    release(self->framebuffer);
    self->framebuffer = framebuffer ? retain(framebuffer) : NULL;
  }
}

/**
 * @fn bool RenderDevice::setSwapchainParameters(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode)
 * @memberof RenderDevice
 */
static bool setSwapchainParameters(const RenderDevice *self, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode) {
  assert(self->window);
  return SDL_SetGPUSwapchainParameters(self->device, self->window, composition, mode);
}

/**
 * @fn void RenderDevice::setWindow(RenderDevice *self, SDL_Window *window)
 * @memberof RenderDevice
 */
static void setWindow(RenderDevice *self, SDL_Window *window) {

  if (self->window == window) {
    return;
  }

  if (self->window) {
    SDL_ReleaseWindowFromGPUDevice(self->device, self->window);
  }

  self->window = window;

  if (window) {
    const bool claimed = SDL_ClaimWindowForGPUDevice(self->device, window);
    GPU_Assert(claimed, "SDL_ClaimWindowForGPUDevice");
  }
}

/**
 * @fn bool RenderDevice::supportsPresentMode(const RenderDevice *self, SDL_GPUPresentMode mode)
 * @memberof RenderDevice
 */
static bool supportsPresentMode(const RenderDevice *self, SDL_GPUPresentMode mode) {
  assert(self->window);
  return SDL_WindowSupportsGPUPresentMode(self->device, self->window, mode);
}

/**
 * @fn bool RenderDevice::supportsSwapchainComposition(const RenderDevice *self, SDL_GPUSwapchainComposition composition)
 * @memberof RenderDevice
 */
static bool supportsSwapchainComposition(const RenderDevice *self, SDL_GPUSwapchainComposition composition) {
  assert(self->window);
  return SDL_WindowSupportsGPUSwapchainComposition(self->device, self->window, composition);
}

/**
 * @fn bool RenderDevice::textureSupportsFormat(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUTextureType type, SDL_GPUTextureUsageFlags usage)
 * @memberof RenderDevice
 */
static bool textureSupportsFormat(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUTextureType type, SDL_GPUTextureUsageFlags usage) {
  return SDL_GPUTextureSupportsFormat(self->device, format, type, usage);
}

/**
 * @fn bool RenderDevice::textureSupportsSampleCount(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUSampleCount sample_count)
 * @memberof RenderDevice
 */
static bool textureSupportsSampleCount(const RenderDevice *self, SDL_GPUTextureFormat format, SDL_GPUSampleCount sample_count) {
  return SDL_GPUTextureSupportsSampleCount(self->device, format, sample_count);
}

/**
 * @fn void RenderDevice::unmapTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf)
 * @memberof RenderDevice
 */
static void unmapTransferBuffer(const RenderDevice *self, SDL_GPUTransferBuffer *tbuf) {
  SDL_UnmapGPUTransferBuffer(self->device, tbuf);
}

/**
 * @fn bool RenderDevice::waitForFences(const RenderDevice *self, bool wait_all, SDL_GPUFence *const *fences, Uint32 num_fences)
 * @memberof RenderDevice
 */
static bool waitForFences(const RenderDevice *self, bool wait_all, SDL_GPUFence *const *fences, Uint32 num_fences) {
  return SDL_WaitForGPUFences(self->device, wait_all, fences, num_fences);
}

/**
 * @fn bool RenderDevice::waitForIdle(const RenderDevice *self)
 * @memberof RenderDevice
 */
static bool waitForIdle(const RenderDevice *self) {
  return SDL_WaitForGPUIdle(self->device);
}

/**
 * @fn bool RenderDevice::waitForSwapchain(const RenderDevice *self)
 * @memberof RenderDevice
 */
static bool waitForSwapchain(const RenderDevice *self) {
  assert(self->window);
  return SDL_WaitForGPUSwapchain(self->device, self->window);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((RenderDeviceInterface *) clazz->interface)->acquireCommandBuffer = acquireCommandBuffer;
  ((RenderDeviceInterface *) clazz->interface)->beginFrame = beginFrame;
  ((RenderDeviceInterface *) clazz->interface)->createBuffer = createBuffer;
  ((RenderDeviceInterface *) clazz->interface)->createBufferWithConstMem = createBufferWithConstMem;
  ((RenderDeviceInterface *) clazz->interface)->createBufferWithData = createBufferWithData;
  ((RenderDeviceInterface *) clazz->interface)->createComputePipeline = createComputePipeline;
  ((RenderDeviceInterface *) clazz->interface)->createFramebuffer = createFramebuffer;
  ((RenderDeviceInterface *) clazz->interface)->createGraphicsPipeline = createGraphicsPipeline;
  ((RenderDeviceInterface *) clazz->interface)->createSampler = createSampler;
  ((RenderDeviceInterface *) clazz->interface)->createShader = createShader;
  ((RenderDeviceInterface *) clazz->interface)->createTexture = createTexture;
  ((RenderDeviceInterface *) clazz->interface)->createTextureFromSurface = createTextureFromSurface;
  ((RenderDeviceInterface *) clazz->interface)->createTransferBuffer = createTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->endFrame = endFrame;
  ((RenderDeviceInterface *) clazz->interface)->getSwapchainTextureFormat = getSwapchainTextureFormat;
  ((RenderDeviceInterface *) clazz->interface)->init = init;
  ((RenderDeviceInterface *) clazz->interface)->initWithWindow = initWithWindow;
  ((RenderDeviceInterface *) clazz->interface)->loadShader = loadShader;
  ((RenderDeviceInterface *) clazz->interface)->loadComputePipeline = loadComputePipeline;
  ((RenderDeviceInterface *) clazz->interface)->mapTransferBuffer = mapTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->queryFence = queryFence;
  ((RenderDeviceInterface *) clazz->interface)->releaseFence = releaseFence;
  ((RenderDeviceInterface *) clazz->interface)->releaseTransferBuffer = releaseTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->setAllowedFramesInFlight = setAllowedFramesInFlight;
  ((RenderDeviceInterface *) clazz->interface)->setFramebuffer = setFramebuffer;
  ((RenderDeviceInterface *) clazz->interface)->setSwapchainParameters = setSwapchainParameters;
  ((RenderDeviceInterface *) clazz->interface)->setWindow = setWindow;
  ((RenderDeviceInterface *) clazz->interface)->supportsPresentMode = supportsPresentMode;
  ((RenderDeviceInterface *) clazz->interface)->supportsSwapchainComposition = supportsSwapchainComposition;
  ((RenderDeviceInterface *) clazz->interface)->textureSupportsFormat = textureSupportsFormat;
  ((RenderDeviceInterface *) clazz->interface)->textureSupportsSampleCount = textureSupportsSampleCount;
  ((RenderDeviceInterface *) clazz->interface)->unmapTransferBuffer = unmapTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->waitForFences = waitForFences;
  ((RenderDeviceInterface *) clazz->interface)->waitForIdle = waitForIdle;
  ((RenderDeviceInterface *) clazz->interface)->waitForSwapchain = waitForSwapchain;
}

/**
 * @fn Class *RenderDevice::_RenderDevice(void)
 * @memberof RenderDevice
 */
Class *_RenderDevice(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "RenderDevice",
      .superclass      = _Object(),
      .instanceSize    = sizeof(RenderDevice),
      .interfaceOffset = offsetof(RenderDevice, interface),
      .interfaceSize   = sizeof(RenderDeviceInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
