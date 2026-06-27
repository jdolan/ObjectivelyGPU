/*
 * ObjectivelyGPU: Object oriented MVC framework for SDL3 GPU, SDL3 and GNU C.
 * Copyright (C) 2014 Jay Dolan <jay@jaydolan.com>
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

#include "CommandBuffer.h"
#include "CopyPass.h"
#include "RenderDevice.h"

#define _Class _RenderDevice

#pragma mark - Object lifecycle

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  RenderDevice *this = (RenderDevice *) self;

  if (this->device) {
    SDL_ReleaseGPUSampler(this->device, this->_samplerNearest);
    SDL_ReleaseGPUSampler(this->device, this->_samplerLinear);
    SDL_ReleaseGPUSampler(this->device, this->_samplerLinearClamp);
    SDL_ReleaseGPUSampler(this->device, this->_samplerAnisotropic);
  }

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

  SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(self->device);
  GPU_Assert(cmd, "SDL_AcquireGPUCommandBuffer");

  CommandBuffer *buffer = $(alloc(CommandBuffer), initWithCommandBuffer, cmd);
  buffer->device = self->device;
  return buffer;
}

/**
 * @fn SDL_GPUBuffer *RenderDevice::createBuffer(const RenderDevice *self, const SDL_GPUBufferCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUBuffer *createBuffer(const RenderDevice *self, const SDL_GPUBufferCreateInfo *info) {

  SDL_GPUBuffer *buffer = SDL_CreateGPUBuffer(self->device, info);
  GPU_Assert(buffer, "SDL_CreateGPUBuffer");

  return buffer;
}

/**
 * @fn SDL_GPUBuffer *RenderDevice::createBufferWithConstMem(const RenderDevice *self, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size)
 * @memberof RenderDevice
 */
static SDL_GPUBuffer *createBufferWithConstMem(const RenderDevice *self, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size) {
  assert(self);
  assert(mem);
  assert(size);

  SDL_GPUBuffer *buffer = $(self, createBuffer, &(SDL_GPUBufferCreateInfo) {
    .usage = usage,
    .size = size,
  });

  SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(self->device, &(SDL_GPUTransferBufferCreateInfo) {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = size,
  });
  GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

  void *mapped = SDL_MapGPUTransferBuffer(self->device, tbuf, false);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  memcpy(mapped, mem, size);
  SDL_UnmapGPUTransferBuffer(self->device, tbuf);

  CommandBuffer *cmd = $(self, acquireCommandBuffer);
  CopyPass *copyPass = $(cmd, beginCopyPass);

  $(copyPass, uploadBuffer,
    &(SDL_GPUTransferBufferLocation) { .transfer_buffer = tbuf },
    &(SDL_GPUBufferRegion) { .buffer = buffer, .size = size },
    false);

  release(copyPass);
  $(self, submit, cmd);
  release(cmd);
  SDL_ReleaseGPUTransferBuffer(self->device, tbuf);

  return buffer;
}

/**
 * @fn SDL_GPUBuffer *RenderDevice::createBufferWithData(const RenderDevice *self, SDL_GPUBufferUsageFlags usage, const Data *data)
 * @memberof RenderDevice
 */
static SDL_GPUBuffer *createBufferWithData(const RenderDevice *self, SDL_GPUBufferUsageFlags usage, const Data *data) {
  assert(self);
  assert(data);

  return $(self, createBufferWithConstMem, usage, data->bytes, (Uint32) data->length);
}

/**
 * @fn SDL_GPUComputePipeline *RenderDevice::createComputePipeline(const RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUComputePipeline *createComputePipeline(const RenderDevice *self, const SDL_GPUComputePipelineCreateInfo *info) {

  SDL_GPUComputePipeline *pipeline = SDL_CreateGPUComputePipeline(self->device, info);
  GPU_Assert(pipeline, "SDL_CreateGPUComputePipeline");

  return pipeline;
}

/**
 * @fn SDL_GPUGraphicsPipeline *RenderDevice::createGraphicsPipeline(const RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUGraphicsPipeline *createGraphicsPipeline(const RenderDevice *self, const SDL_GPUGraphicsPipelineCreateInfo *info) {

  SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(self->device, info);
  GPU_Assert(pipeline, "SDL_CreateGPUGraphicsPipeline");

  return pipeline;
}

/**
 * @fn SDL_GPUSampler *RenderDevice::createSampler(const RenderDevice *self, const SDL_GPUSamplerCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUSampler *createSampler(const RenderDevice *self, const SDL_GPUSamplerCreateInfo *info) {

  SDL_GPUSampler *sampler = SDL_CreateGPUSampler(self->device, info);
  GPU_Assert(sampler, "SDL_CreateGPUSampler");

  return sampler;
}

/**
 * @fn SDL_GPUSampler *RenderDevice::samplerAnisotropic(const RenderDevice *self)
 * @memberof RenderDevice
 */
static SDL_GPUSampler *samplerAnisotropic(const RenderDevice *self) {
  assert(self);

  RenderDevice *this = (RenderDevice *) self;
  if (!this->_samplerAnisotropic) {
    this->_samplerAnisotropic = $(self, createSampler, &(SDL_GPUSamplerCreateInfo) {
      .min_filter = SDL_GPU_FILTER_LINEAR,
      .mag_filter = SDL_GPU_FILTER_LINEAR,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .enable_anisotropy = true,
      .max_anisotropy = 16.f,
    });
  }
  return this->_samplerAnisotropic;
}

/**
 * @fn SDL_GPUSampler *RenderDevice::samplerLinear(const RenderDevice *self)
 * @memberof RenderDevice
 */
static SDL_GPUSampler *samplerLinear(const RenderDevice *self) {
  assert(self);

  RenderDevice *this = (RenderDevice *) self;
  if (!this->_samplerLinear) {
    this->_samplerLinear = $(self, createSampler, &(SDL_GPUSamplerCreateInfo) {
      .min_filter = SDL_GPU_FILTER_LINEAR,
      .mag_filter = SDL_GPU_FILTER_LINEAR,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    });
  }
  return this->_samplerLinear;
}

/**
 * @fn SDL_GPUSampler *RenderDevice::samplerLinearClamp(const RenderDevice *self)
 * @memberof RenderDevice
 */
static SDL_GPUSampler *samplerLinearClamp(const RenderDevice *self) {
  assert(self);

  RenderDevice *this = (RenderDevice *) self;
  if (!this->_samplerLinearClamp) {
    this->_samplerLinearClamp = $(self, createSampler, &(SDL_GPUSamplerCreateInfo) {
      .min_filter = SDL_GPU_FILTER_LINEAR,
      .mag_filter = SDL_GPU_FILTER_LINEAR,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    });
  }
  return this->_samplerLinearClamp;
}

/**
 * @fn SDL_GPUSampler *RenderDevice::samplerNearest(const RenderDevice *self)
 * @memberof RenderDevice
 */
static SDL_GPUSampler *samplerNearest(const RenderDevice *self) {
  assert(self);

  RenderDevice *this = (RenderDevice *) self;
  if (!this->_samplerNearest) {
    this->_samplerNearest = $(self, createSampler, &(SDL_GPUSamplerCreateInfo) {
      .min_filter = SDL_GPU_FILTER_NEAREST,
      .mag_filter = SDL_GPU_FILTER_NEAREST,
      .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
      .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    });
  }
  return this->_samplerNearest;
}

/**
 * @fn SDL_GPUShader *RenderDevice::createShader(const RenderDevice *self, const SDL_GPUShaderCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUShader *createShader(const RenderDevice *self, const SDL_GPUShaderCreateInfo *info) {

  SDL_GPUShader *shader = SDL_CreateGPUShader(self->device, info);
  GPU_Assert(shader, "SDL_CreateGPUShader");

  return shader;
}

/**
 * @fn SDL_GPUTexture *RenderDevice::createTexture(const RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels)
 * @memberof RenderDevice
 */
static SDL_GPUTexture *createTexture(const RenderDevice *self, const SDL_GPUTextureCreateInfo *info, const void *pixels) {

  SDL_GPUTexture *texture = SDL_CreateGPUTexture(self->device, info);
  GPU_Assert(texture, "SDL_CreateGPUTexture");

  if (pixels) {
    const Uint32 bytesPerTexel = SDL_GPUTextureFormatTexelBlockSize(info->format);
    const Uint32 totalBytes = info->width * info->height * info->layer_count_or_depth * bytesPerTexel;

    const SDL_GPUTransferBufferCreateInfo tbufInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = totalBytes,
    };

    SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(self->device, &tbufInfo);
    GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

    void *mapped = SDL_MapGPUTransferBuffer(self->device, tbuf, false);
    GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

    memcpy(mapped, pixels, totalBytes);
    SDL_UnmapGPUTransferBuffer(self->device, tbuf);

    CommandBuffer *cmd = $(self, acquireCommandBuffer);
    CopyPass *copyPass = $(cmd, beginCopyPass);

    const SDL_GPUTextureTransferInfo src = {
      .transfer_buffer = tbuf,
      .offset = 0,
      .pixels_per_row = info->width,
      .rows_per_layer = info->height,
    };
    const SDL_GPUTextureRegion dst = {
      .texture = texture,
      .w = info->width,
      .h = info->height,
      .d = info->layer_count_or_depth,
    };
    $(copyPass, uploadTexture, &src, &dst, false);

    release(copyPass);
    $(self, submit, cmd);
    release(cmd);
    SDL_ReleaseGPUTransferBuffer(self->device, tbuf);
  }

  return texture;
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
 * @fn SDL_GPUTexture *RenderDevice::createTextureFromSurface(const RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage)
 * @memberof RenderDevice
 */
static SDL_GPUTexture *createTextureFromSurface(const RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage) {
  assert(self);
  assert(surface);

  SDL_Surface *rgba = surface->format == SDL_PIXELFORMAT_RGBA32
    ? surface
    : SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  GPU_Assert(rgba, "SDL_ConvertSurface");

  SDL_GPUTexture *texture = $(self, createTexture, &(SDL_GPUTextureCreateInfo) {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = usage,
    .width = (Uint32) rgba->w,
    .height = (Uint32) rgba->h,
    .layer_count_or_depth = 1,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  }, rgba->pixels);

  if (rgba != surface) {
    SDL_DestroySurface(rgba);
  }

  return texture;
}
static SDL_GPUTextureFormat getSwapchainTextureFormat(const RenderDevice *self, SDL_Window *window) {
  return SDL_GetGPUSwapchainTextureFormat(self->device, window);
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

    self->device = SDL_CreateGPUDevice(formats, false, NULL);
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
 * @fn SDL_GPUShader *RenderDevice::loadShader(const RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUShader *loadShader(const RenderDevice *self, const char *name, const SDL_GPUShaderCreateInfo *info) {

  static const struct {
    SDL_GPUShaderFormat format;
    const char *ext;
  } formats[] = {
    { SDL_GPU_SHADERFORMAT_MSL,  ".msl"  },
    { SDL_GPU_SHADERFORMAT_DXIL, ".dxil" },
    { SDL_GPU_SHADERFORMAT_SPIRV,".spv"  },
  };

  const SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(self->device);

  for (size_t i = 0; i < SDL_arraysize(formats); i++) {
    if (!(supported & formats[i].format)) {
      continue;
    }

    char path[256];
    SDL_snprintf(path, sizeof(path), "%s%s", name, formats[i].ext);

    Resource *res = $$(Resource, resourceWithName, path);
    if (!res) {
      continue;
    }

    SDL_GPUShaderCreateInfo filled = *info;
    filled.code = res->data->bytes;
    filled.code_size = res->data->length;
    filled.format = formats[i].format;

    SDL_GPUShader *shader = $(self, createShader, &filled);
    release(res);
    return shader;
  }

  GPU_Assert(false, "loadShader: no supported format found for '%s'\n", name);
  return NULL;
}

/**
 * @fn SDL_GPUComputePipeline *RenderDevice::loadComputePipeline(const RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static SDL_GPUComputePipeline *loadComputePipeline(const RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info) {

  static const struct {
    SDL_GPUShaderFormat format;
    const char *ext;
  } formats[] = {
    { SDL_GPU_SHADERFORMAT_MSL,  ".msl"  },
    { SDL_GPU_SHADERFORMAT_DXIL, ".dxil" },
    { SDL_GPU_SHADERFORMAT_SPIRV,".spv"  },
  };

  const SDL_GPUShaderFormat supported = SDL_GetGPUShaderFormats(self->device);

  for (size_t i = 0; i < SDL_arraysize(formats); i++) {

    if (!(supported & formats[i].format)) {
      continue;
    }

    char path[256];
    SDL_snprintf(path, sizeof(path), "%s%s", name, formats[i].ext);

    Resource *res = $$(Resource, resourceWithName, path);
    if (!res) {
      continue;
    }

    if (!res->data || res->data->length == 0) {
      release(res);
      continue;
    }

    SDL_GPUComputePipelineCreateInfo filled = *info;
    filled.code = res->data->bytes;
    filled.code_size = res->data->length;
    filled.format = formats[i].format;

    SDL_GPUComputePipeline *pipeline = $(self, createComputePipeline, &filled);
    release(res);
    return pipeline;
  }

  GPU_Assert(false, "loadComputePipeline: no shader blob found for '%s' in any supported format", name);
  return NULL;
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
 * @fn void RenderDevice::releaseBuffer(const RenderDevice *self, SDL_GPUBuffer *buffer)
 * @memberof RenderDevice
 */
static void releaseBuffer(const RenderDevice *self, SDL_GPUBuffer *buffer) {
  SDL_ReleaseGPUBuffer(self->device, buffer);
}

/**
 * @fn void RenderDevice::releaseComputePipeline(const RenderDevice *self, SDL_GPUComputePipeline *pipeline)
 * @memberof RenderDevice
 */
static void releaseComputePipeline(const RenderDevice *self, SDL_GPUComputePipeline *pipeline) {
  SDL_ReleaseGPUComputePipeline(self->device, pipeline);
}

/**
 * @fn void RenderDevice::releaseFence(const RenderDevice *self, SDL_GPUFence *fence)
 * @memberof RenderDevice
 */
static void releaseFence(const RenderDevice *self, SDL_GPUFence *fence) {
  SDL_ReleaseGPUFence(self->device, fence);
}

/**
 * @fn void RenderDevice::releaseGraphicsPipeline(const RenderDevice *self, SDL_GPUGraphicsPipeline *pipeline)
 * @memberof RenderDevice
 */
static void releaseGraphicsPipeline(const RenderDevice *self, SDL_GPUGraphicsPipeline *pipeline) {
  SDL_ReleaseGPUGraphicsPipeline(self->device, pipeline);
}

/**
 * @fn void RenderDevice::releaseSampler(const RenderDevice *self, SDL_GPUSampler *sampler)
 * @memberof RenderDevice
 */
static void releaseSampler(const RenderDevice *self, SDL_GPUSampler *sampler) {
  SDL_ReleaseGPUSampler(self->device, sampler);
}

/**
 * @fn void RenderDevice::releaseShader(const RenderDevice *self, SDL_GPUShader *shader)
 * @memberof RenderDevice
 */
static void releaseShader(const RenderDevice *self, SDL_GPUShader *shader) {
  SDL_ReleaseGPUShader(self->device, shader);
}

/**
 * @fn void RenderDevice::releaseTexture(const RenderDevice *self, SDL_GPUTexture *texture)
 * @memberof RenderDevice
 */
static void releaseTexture(const RenderDevice *self, SDL_GPUTexture *texture) {
  SDL_ReleaseGPUTexture(self->device, texture);
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
 * @fn void RenderDevice::setBufferName(const RenderDevice *self, SDL_GPUBuffer *buffer, const char *name)
 * @memberof RenderDevice
 */
static void setBufferName(const RenderDevice *self, SDL_GPUBuffer *buffer, const char *name) {
  SDL_SetGPUBufferName(self->device, buffer, name);
}

/**
 * @fn bool RenderDevice::setSwapchainParameters(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode)
 * @memberof RenderDevice
 */
static bool setSwapchainParameters(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition, SDL_GPUPresentMode mode) {
  return SDL_SetGPUSwapchainParameters(self->device, window, composition, mode);
}

/**
 * @fn void RenderDevice::setTextureName(const RenderDevice *self, SDL_GPUTexture *texture, const char *name)
 * @memberof RenderDevice
 */
static void setTextureName(const RenderDevice *self, SDL_GPUTexture *texture, const char *name) {
  SDL_SetGPUTextureName(self->device, texture, name);
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
 * @fn void RenderDevice::submit(const RenderDevice *self, CommandBuffer *cmd)
 * @memberof RenderDevice
 */
static void submit(const RenderDevice *self, CommandBuffer *cmd) {
  const bool ok = SDL_SubmitGPUCommandBuffer(cmd->cmd);
  GPU_Assert(ok, "SDL_SubmitGPUCommandBuffer");
}

/**
 * @fn SDL_GPUFence *RenderDevice::submitAndFence(const RenderDevice *self, CommandBuffer *cmd)
 * @memberof RenderDevice
 */
static SDL_GPUFence *submitAndFence(const RenderDevice *self, CommandBuffer *cmd) {

  SDL_GPUFence *fence = SDL_SubmitGPUCommandBufferAndAcquireFence(cmd->cmd);
  GPU_Assert(fence, "SDL_SubmitGPUCommandBufferAndAcquireFence");

  return fence;
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
 * @fn void RenderDevice::uploadBuffer(const RenderDevice *self, SDL_GPUBuffer *buffer, const void *data, Uint32 size, Uint32 offset, bool cycle)
 * @memberof RenderDevice
 */
static void uploadBuffer(const RenderDevice *self, SDL_GPUBuffer *buffer, const void *data, Uint32 size, Uint32 offset, bool cycle) {
  assert(self);
  assert(buffer);
  assert(data);
  assert(size);

  SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(self->device, &(SDL_GPUTransferBufferCreateInfo) {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = size,
  });
  GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

  void *mapped = SDL_MapGPUTransferBuffer(self->device, tbuf, cycle);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  memcpy(mapped, data, size);
  SDL_UnmapGPUTransferBuffer(self->device, tbuf);

  CommandBuffer *cmd = $(self, acquireCommandBuffer);
  CopyPass *copyPass = $(cmd, beginCopyPass);

  $(copyPass, uploadBuffer,
    &(SDL_GPUTransferBufferLocation) { .transfer_buffer = tbuf },
    &(SDL_GPUBufferRegion) { .buffer = buffer, .offset = offset, .size = size },
    cycle);

  release(copyPass);
  $(self, submit, cmd);
  release(cmd);
  SDL_ReleaseGPUTransferBuffer(self->device, tbuf);
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
 * @fn bool RenderDevice::waitForSwapchain(const RenderDevice *self, SDL_Window *window)
 * @memberof RenderDevice
 */
static bool waitForSwapchain(const RenderDevice *self, SDL_Window *window) {
  return SDL_WaitForGPUSwapchain(self->device, window);
}

/**
 * @fn bool RenderDevice::windowSupportsPresentMode(const RenderDevice *self, SDL_Window *window, SDL_GPUPresentMode mode)
 * @memberof RenderDevice
 */
static bool windowSupportsPresentMode(const RenderDevice *self, SDL_Window *window, SDL_GPUPresentMode mode) {
  return SDL_WindowSupportsGPUPresentMode(self->device, window, mode);
}

/**
 * @fn bool RenderDevice::windowSupportsSwapchainComposition(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition)
 * @memberof RenderDevice
 */
static bool windowSupportsSwapchainComposition(const RenderDevice *self, SDL_Window *window, SDL_GPUSwapchainComposition composition) {
  return SDL_WindowSupportsGPUSwapchainComposition(self->device, window, composition);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((RenderDeviceInterface *) clazz->interface)->acquireCommandBuffer = acquireCommandBuffer;
  ((RenderDeviceInterface *) clazz->interface)->createBuffer = createBuffer;
  ((RenderDeviceInterface *) clazz->interface)->createBufferWithConstMem = createBufferWithConstMem;
  ((RenderDeviceInterface *) clazz->interface)->createBufferWithData = createBufferWithData;
  ((RenderDeviceInterface *) clazz->interface)->createComputePipeline = createComputePipeline;
  ((RenderDeviceInterface *) clazz->interface)->createGraphicsPipeline = createGraphicsPipeline;
  ((RenderDeviceInterface *) clazz->interface)->createSampler = createSampler;
  ((RenderDeviceInterface *) clazz->interface)->createShader = createShader;
  ((RenderDeviceInterface *) clazz->interface)->createTexture = createTexture;
  ((RenderDeviceInterface *) clazz->interface)->createTextureFromSurface = createTextureFromSurface;
  ((RenderDeviceInterface *) clazz->interface)->createTransferBuffer = createTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->getSwapchainTextureFormat = getSwapchainTextureFormat;
  ((RenderDeviceInterface *) clazz->interface)->init = init;
  ((RenderDeviceInterface *) clazz->interface)->initWithWindow = initWithWindow;
  ((RenderDeviceInterface *) clazz->interface)->loadShader = loadShader;
  ((RenderDeviceInterface *) clazz->interface)->loadComputePipeline = loadComputePipeline;
  ((RenderDeviceInterface *) clazz->interface)->mapTransferBuffer = mapTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->queryFence = queryFence;
  ((RenderDeviceInterface *) clazz->interface)->releaseBuffer = releaseBuffer;
  ((RenderDeviceInterface *) clazz->interface)->releaseComputePipeline = releaseComputePipeline;
  ((RenderDeviceInterface *) clazz->interface)->releaseFence = releaseFence;
  ((RenderDeviceInterface *) clazz->interface)->releaseGraphicsPipeline = releaseGraphicsPipeline;
  ((RenderDeviceInterface *) clazz->interface)->releaseSampler = releaseSampler;
  ((RenderDeviceInterface *) clazz->interface)->releaseShader = releaseShader;
  ((RenderDeviceInterface *) clazz->interface)->releaseTexture = releaseTexture;
  ((RenderDeviceInterface *) clazz->interface)->releaseTransferBuffer = releaseTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->samplerAnisotropic = samplerAnisotropic;
  ((RenderDeviceInterface *) clazz->interface)->samplerLinear = samplerLinear;
  ((RenderDeviceInterface *) clazz->interface)->samplerLinearClamp = samplerLinearClamp;
  ((RenderDeviceInterface *) clazz->interface)->samplerNearest = samplerNearest;
  ((RenderDeviceInterface *) clazz->interface)->setAllowedFramesInFlight = setAllowedFramesInFlight;
  ((RenderDeviceInterface *) clazz->interface)->setBufferName = setBufferName;
  ((RenderDeviceInterface *) clazz->interface)->setSwapchainParameters = setSwapchainParameters;
  ((RenderDeviceInterface *) clazz->interface)->setTextureName = setTextureName;
  ((RenderDeviceInterface *) clazz->interface)->setWindow = setWindow;
  ((RenderDeviceInterface *) clazz->interface)->submit = submit;
  ((RenderDeviceInterface *) clazz->interface)->submitAndFence = submitAndFence;
  ((RenderDeviceInterface *) clazz->interface)->textureSupportsFormat = textureSupportsFormat;
  ((RenderDeviceInterface *) clazz->interface)->textureSupportsSampleCount = textureSupportsSampleCount;
  ((RenderDeviceInterface *) clazz->interface)->unmapTransferBuffer = unmapTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->uploadBuffer = uploadBuffer;
  ((RenderDeviceInterface *) clazz->interface)->waitForFences = waitForFences;
  ((RenderDeviceInterface *) clazz->interface)->waitForIdle = waitForIdle;
  ((RenderDeviceInterface *) clazz->interface)->waitForSwapchain = waitForSwapchain;
  ((RenderDeviceInterface *) clazz->interface)->windowSupportsPresentMode = windowSupportsPresentMode;
  ((RenderDeviceInterface *) clazz->interface)->windowSupportsSwapchainComposition = windowSupportsSwapchainComposition;
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
