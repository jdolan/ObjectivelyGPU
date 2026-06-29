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

#include "Buffer.h"
#include "CommandBuffer.h"
#include "CopyPass.h"
#include "RenderDevice.h"

#define _Class _Buffer

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  Buffer *this = (Buffer *) self;

  if (this->device) {
    SDL_ReleaseGPUBuffer(this->device->device, this->buffer);
    release(this->device);
  }

  super(Object, self, dealloc);
}

#pragma mark - Buffer

/**
 * @fn Buffer *Buffer::initWithConstMem(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size)
 * @memberof Buffer
 */
static Buffer *initWithConstMem(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size) {

  assert(device);
  assert(mem);
  assert(size);

  self = $(self, initWithDevice, device, &(SDL_GPUBufferCreateInfo) {
    .usage = usage,
    .size = size,
  });

  if (self) {
    $(self, upload, mem, size, 0, false);
  }

  return self;
}

/**
 * @fn Buffer *Buffer::initWithData(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const Data *data)
 * @memberof Buffer
 */
static Buffer *initWithData(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const Data *data) {

  assert(device);
  assert(data);

  return $(self, initWithConstMem, device, usage, data->bytes, (Uint32) data->length);
}

/**
 * @fn Buffer *Buffer::initWithDevice(Buffer *self, RenderDevice *device, const SDL_GPUBufferCreateInfo *info)
 * @memberof Buffer
 */
static Buffer *initWithDevice(Buffer *self, RenderDevice *device, const SDL_GPUBufferCreateInfo *info) {

  assert(device);
  assert(info);

  self = (Buffer *) super(Object, self, init);
  if (self) {

    self->device = retain(device);

    self->buffer = SDL_CreateGPUBuffer(device->device, info);
    GPU_Assert(self->buffer, "SDL_CreateGPUBuffer");

    self->usage = info->usage;
    self->size = info->size;
  }

  return self;
}

/**
 * @fn void Buffer::setName(Buffer *self, const char *name)
 * @memberof Buffer
 */
static void setName(Buffer *self, const char *name) {

  assert(self);

  SDL_SetGPUBufferName(self->device->device, self->buffer, name);
}

/**
 * @fn void Buffer::upload(Buffer *self, const void *data, Uint32 size, Uint32 offset, bool cycle)
 * @memberof Buffer
 */
static void upload(Buffer *self, const void *data, Uint32 size, Uint32 offset, bool cycle) {

  assert(self);
  assert(data);
  assert(size);

  SDL_GPUTransferBuffer *tbuf = SDL_CreateGPUTransferBuffer(self->device->device, &(SDL_GPUTransferBufferCreateInfo) {
    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    .size = size,
  });
  GPU_Assert(tbuf, "SDL_CreateGPUTransferBuffer");

  void *mapped = SDL_MapGPUTransferBuffer(self->device->device, tbuf, cycle);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  memcpy(mapped, data, size);
  SDL_UnmapGPUTransferBuffer(self->device->device, tbuf);

  CommandBuffer *commands = $(self->device, acquireCommandBuffer);
  CopyPass *copyPass = $(commands, beginCopyPass);

  $(copyPass, uploadBuffer,
    &(SDL_GPUTransferBufferLocation) { .transfer_buffer = tbuf },
    &(SDL_GPUBufferRegion) { .buffer = self->buffer, .offset = offset, .size = size },
    cycle);

  release(copyPass);
  $(self->device, submit, commands);
  release(commands);
  SDL_ReleaseGPUTransferBuffer(self->device->device, tbuf);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((BufferInterface *) clazz->interface)->initWithConstMem = initWithConstMem;
  ((BufferInterface *) clazz->interface)->initWithData = initWithData;
  ((BufferInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((BufferInterface *) clazz->interface)->setName = setName;
  ((BufferInterface *) clazz->interface)->upload = upload;
}

/**
 * @fn Class *Buffer::_Buffer(void)
 * @memberof Buffer
 */
Class *_Buffer(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "Buffer",
      .superclass      = _Object(),
      .instanceSize    = sizeof(Buffer),
      .interfaceOffset = offsetof(Buffer, interface),
      .interfaceSize   = sizeof(BufferInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
