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
#include <string.h>

#include <SDL3/SDL_surface.h>

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

/**
 * @brief Metadata for a shader binary format: its SDL enum, the file extension
 * of the transpiled blob, and the entry-point name the toolchain emits for each
 * stage. shadercross names every MSL (and hence metallib) entry point "main0";
 * SPIR-V and DXIL keep the GLSL/HLSL default "main".
 * @see RenderDevice::loadShader
 */
typedef struct {
  SDL_GPUShaderFormat format;
  const char *ext;
  const char *vertexEntrypoint;
  const char *fragmentEntrypoint;
  const char *computeEntrypoint;
} GPU_ShaderFormat;

/**
 * @brief The shader formats supported by ObjectivelyGPU.
 */
static const GPU_ShaderFormat GPU_ShaderFormats[] = {
  { SDL_GPU_SHADERFORMAT_METALLIB, ".metallib", "main0", "main0", "main0" },
  { SDL_GPU_SHADERFORMAT_MSL,      ".metal",    "main0", "main0", "main0" },
  { SDL_GPU_SHADERFORMAT_DXIL,     ".dxil",     "main",  "main",  "main"  },
  { SDL_GPU_SHADERFORMAT_SPIRV,    ".spv",      "main",  "main",  "main"  },
};

/**
 * @brief A helper
 */
static Resource *resolveShaderResource(const char *name, SDL_GPUShaderFormat supported, GPU_ShaderFormat *resolved) {

  for (size_t i = 0; i < lengthof(GPU_ShaderFormats); i++) {
    
    const GPU_ShaderFormat *fmt = &GPU_ShaderFormats[i];
    if (!(fmt->format & supported)) {
      continue;
    }
    
    char path[256];
    SDL_snprintf(path, sizeof(path), "%s%s", name, fmt->ext);

    Resource *res = $$(Resource, resourceWithName, path);
    if (!res) {
      continue;
    }

    *resolved = *fmt;
    return res;
  }

  GPU_Assert(false, "Failed to resolve shader '%s'", name);
  return NULL;
}

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
  GPU_Assert(self->commands == NULL, "beginFrame called with a frame already in flight");

  self->commands = $(self, acquireCommandBuffer);

  const bool ok = $(self->commands, waitAndAcquireSwapchainTexture, &self->swapchain);
  if (ok) {
    $(self->framebuffer, resize, &self->swapchain.size);
  } else {
    $(self->commands, cancel);
    self->commands = release(self->commands);
  }

  return self->commands;
}

/**
 * @fn void RenderDevice::endFrame(RenderDevice *self)
 * @memberof RenderDevice
 */
static void endFrame(RenderDevice *self) {

  GPU_Assert(self->commands, "endFrame called without a frame in flight");

  Texture *color = $(self->framebuffer, resolveColorTexture, 0);
  GPU_Assert(color, "framebuffer has no color attachment to present");

  $(self->commands, blitTexture, &(SDL_GPUBlitInfo) {
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

  $(self->commands, submit);

  self->commands = release(self->commands);
  
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
 * @fn Sampler *RenderDevice::createSamplerLinearRepeat(RenderDevice *self, float maxAnisotropy)
 * @memberof RenderDevice
 */
static Sampler *createSamplerLinearRepeat(RenderDevice *self, float maxAnisotropy) {

  return $(self, createSampler, &(const SDL_GPUSamplerCreateInfo) {
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    .enable_anisotropy = maxAnisotropy > 0.f,
    .max_anisotropy = maxAnisotropy,
  });
}

/**
 * @fn Sampler *RenderDevice::createSamplerLinearClamp(RenderDevice *self)
 * @memberof RenderDevice
 */
static Sampler *createSamplerLinearClamp(RenderDevice *self) {

  return $(self, createSampler, &(const SDL_GPUSamplerCreateInfo) {
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  });
}

/**
 * @fn Sampler *RenderDevice::createSamplerNearestClamp(RenderDevice *self)
 * @memberof RenderDevice
 */
static Sampler *createSamplerNearestClamp(RenderDevice *self) {

  return $(self, createSampler, &(const SDL_GPUSamplerCreateInfo) {
    .min_filter = SDL_GPU_FILTER_NEAREST,
    .mag_filter = SDL_GPU_FILTER_NEAREST,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
  });
}

/**
 * @fn Sampler *RenderDevice::createSamplerShadowCompare(RenderDevice *self)
 * @memberof RenderDevice
 */
static Sampler *createSamplerShadowCompare(RenderDevice *self) {

  return $(self, createSampler, &(const SDL_GPUSamplerCreateInfo) {
    .min_filter = SDL_GPU_FILTER_LINEAR,
    .mag_filter = SDL_GPU_FILTER_LINEAR,
    .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
    .compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
    .enable_compare = true,
  });
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
static Texture *createTextureFromSurface(RenderDevice *self, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage,
                                          bool mipmaps) {

  return $(alloc(Texture), initWithSurface, self, surface, usage, mipmaps);
}

/**
 * @fn Texture *RenderDevice::createSolidColorTexture(RenderDevice *self, SDL_GPUTextureType type, Uint32 layerCount, Uint32 rgba)
 * @memberof RenderDevice
 */
static Texture *createSolidColorTexture(RenderDevice *self, SDL_GPUTextureType type, Uint32 layerCount, Uint32 rgba) {

  assert(type == SDL_GPU_TEXTURETYPE_2D || type == SDL_GPU_TEXTURETYPE_CUBE);
  assert(layerCount >= 1 && layerCount <= 6);

  Uint32 pixels[6];
  for (Uint32 i = 0; i < layerCount; i++) {
    pixels[i] = rgba;
  }

  return $(self, createTexture, &(const SDL_GPUTextureCreateInfo) {
    .type = type,
    .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = 1,
    .height = 1,
    .layer_count_or_depth = layerCount,
    .num_levels = 1,
    .sample_count = SDL_GPU_SAMPLECOUNT_1,
  }, pixels);
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

    const SDL_GPUShaderFormat requested =
      SDL_GPU_SHADERFORMAT_METALLIB |
      SDL_GPU_SHADERFORMAT_MSL |
      SDL_GPU_SHADERFORMAT_SPIRV |
      SDL_GPU_SHADERFORMAT_DXIL;

    self->device = SDL_CreateGPUDevice(requested, GPU_DEBUG, NULL);
    GPU_Assert(self->device, "SDL_CreateGPUDevice");
    
    self->shaderFormats = SDL_GetGPUShaderFormats(self->device);
    GPU_Assert(self->shaderFormats, "SDL_GetGPUShaderFormats");
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

  GPU_ShaderFormat resolved;
  Resource *resource = resolveShaderResource(name, self->shaderFormats, &resolved);

  SDL_Log("Compiling shader \"%s%s\" (%s, %u samplers, %u uniform buffers, %u storage buffers)",
          name, resolved.ext,
          info->stage == SDL_GPU_SHADERSTAGE_VERTEX ? "vertex" : "fragment",
          info->num_samplers, info->num_uniform_buffers, info->num_storage_buffers);

  SDL_GPUShaderCreateInfo create = *info;

  create.code = resource->data->bytes;
  create.code_size = resource->data->length;
  create.format = resolved.format;

  if (!create.entrypoint) {
    switch (create.stage) {
      case SDL_GPU_SHADERSTAGE_VERTEX:
        create.entrypoint = resolved.vertexEntrypoint;
        break;
      case SDL_GPU_SHADERSTAGE_FRAGMENT:
        create.entrypoint = resolved.fragmentEntrypoint;
        break;
    }
  }

  Shader *shader = $(alloc(Shader), initWithDevice, self, &create);

  release(resource);
  return shader;
}

/**
 * @fn ComputePipeline *RenderDevice::loadComputePipeline(RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static ComputePipeline *loadComputePipeline(RenderDevice *self, const char *name, const SDL_GPUComputePipelineCreateInfo *info) {

  GPU_ShaderFormat resolved;
  Resource *resource = resolveShaderResource(name, self->shaderFormats, &resolved);

  SDL_Log("Compiling compute pipeline \"%s%s\" (threads %ux%ux%u)",
          name, resolved.ext,
          info->threadcount_x, info->threadcount_y, info->threadcount_z);

  SDL_GPUComputePipelineCreateInfo create = *info;
  create.code = resource->data->bytes;
  create.code_size = resource->data->length;
  create.format = resolved.format;

  if (!create.entrypoint) {
    create.entrypoint = resolved.computeEntrypoint;
  }

  ComputePipeline *pipeline = $(alloc(ComputePipeline), initWithDevice, self, &create);

  release(resource);
  return pipeline;
}

/**
 * @fn GraphicsPipeline *RenderDevice::loadGraphicsPipeline(RenderDevice *self, const char *vertexShaderName, const SDL_GPUShaderCreateInfo *vertexShaderInfo, const char *fragmentShaderName, const SDL_GPUShaderCreateInfo *fragmentShaderInfo, SDL_GPUGraphicsPipelineCreateInfo *info)
 * @memberof RenderDevice
 */
static GraphicsPipeline *loadGraphicsPipeline(RenderDevice *self,
                                               const char *vertexShaderName, const SDL_GPUShaderCreateInfo *vertexShaderInfo,
                                               const char *fragmentShaderName, const SDL_GPUShaderCreateInfo *fragmentShaderInfo,
                                               SDL_GPUGraphicsPipelineCreateInfo *info) {

  Shader *vertexShader = $(self, loadShader, vertexShaderName, vertexShaderInfo);
  Shader *fragmentShader = $(self, loadShader, fragmentShaderName, fragmentShaderInfo);

  info->vertex_shader = vertexShader->shader;
  info->fragment_shader = fragmentShader->shader;

  GraphicsPipeline *pipeline = $(self, createGraphicsPipeline, info);

  release(vertexShader);
  release(fragmentShader);

  return pipeline;
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
    self->framebuffer = release(self->framebuffer);
    
    if (framebuffer) {
      self->framebuffer = retain(framebuffer);
    }
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
  ((RenderDeviceInterface *) clazz->interface)->createSamplerLinearRepeat = createSamplerLinearRepeat;
  ((RenderDeviceInterface *) clazz->interface)->createSamplerLinearClamp = createSamplerLinearClamp;
  ((RenderDeviceInterface *) clazz->interface)->createSamplerNearestClamp = createSamplerNearestClamp;
  ((RenderDeviceInterface *) clazz->interface)->createSamplerShadowCompare = createSamplerShadowCompare;
  ((RenderDeviceInterface *) clazz->interface)->createShader = createShader;
  ((RenderDeviceInterface *) clazz->interface)->createTexture = createTexture;
  ((RenderDeviceInterface *) clazz->interface)->createTextureFromSurface = createTextureFromSurface;
  ((RenderDeviceInterface *) clazz->interface)->createSolidColorTexture = createSolidColorTexture;
  ((RenderDeviceInterface *) clazz->interface)->createTransferBuffer = createTransferBuffer;
  ((RenderDeviceInterface *) clazz->interface)->endFrame = endFrame;
  ((RenderDeviceInterface *) clazz->interface)->getSwapchainTextureFormat = getSwapchainTextureFormat;
  ((RenderDeviceInterface *) clazz->interface)->init = init;
  ((RenderDeviceInterface *) clazz->interface)->initWithWindow = initWithWindow;
  ((RenderDeviceInterface *) clazz->interface)->loadShader = loadShader;
  ((RenderDeviceInterface *) clazz->interface)->loadComputePipeline = loadComputePipeline;
  ((RenderDeviceInterface *) clazz->interface)->loadGraphicsPipeline = loadGraphicsPipeline;
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
