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

#include "RenderDevice.h"
#include "TransferBuffer.h"

#define _Class _TransferBuffer

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  TransferBuffer *this = (TransferBuffer *) self;

  if (this->device) {
    if (this->buffer) {
      SDL_ReleaseGPUTransferBuffer(this->device->device, this->buffer);
    }
  }

  super(Object, self, dealloc);
}

#pragma mark - TransferBuffer

/**
 * @fn TransferBuffer *TransferBuffer::initWithDevice(TransferBuffer *self, RenderDevice *device, const SDL_GPUTransferBufferCreateInfo *info)
 * @memberof TransferBuffer
 */
static TransferBuffer *initWithDevice(TransferBuffer *self, RenderDevice *device, const SDL_GPUTransferBufferCreateInfo *info) {

  assert(device);
  assert(info);

  self = (TransferBuffer *) super(Object, self, init);
  if (self) {

    self->device = device;
    self->size = info->size;

    self->buffer = SDL_CreateGPUTransferBuffer(device->device, info);
    GPU_Assert(self->buffer, "SDL_CreateGPUTransferBuffer");
  }

  return self;
}

/**
 * @fn void *TransferBuffer::map(const TransferBuffer *self, bool cycle)
 * @memberof TransferBuffer
 */
static void *map(const TransferBuffer *self, bool cycle) {

  void *mapped = SDL_MapGPUTransferBuffer(self->device->device, self->buffer, cycle);
  GPU_Assert(mapped, "SDL_MapGPUTransferBuffer");

  return mapped;
}

/**
 * @fn void TransferBuffer::unmap(const TransferBuffer *self)
 * @memberof TransferBuffer
 */
static void unmap(const TransferBuffer *self) {
  SDL_UnmapGPUTransferBuffer(self->device->device, self->buffer);
}

/**
 * @fn void TransferBuffer::write(const TransferBuffer *self, const void *data, Uint32 size, bool cycle)
 * @memberof TransferBuffer
 */
static void writeData(const TransferBuffer *self, const void *data, Uint32 size, bool cycle) {

  assert(data);
  assert(size <= self->size);

  void *mapped = $(self, map, cycle);
  memcpy(mapped, data, size);
  $(self, unmap);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((TransferBufferInterface *) clazz->interface)->initWithDevice = initWithDevice;
  ((TransferBufferInterface *) clazz->interface)->map = map;
  ((TransferBufferInterface *) clazz->interface)->unmap = unmap;
  ((TransferBufferInterface *) clazz->interface)->write = writeData;
}

/**
 * @fn Class *TransferBuffer::_TransferBuffer(void)
 * @memberof TransferBuffer
 */
Class *_TransferBuffer(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "TransferBuffer",
      .superclass      = _Object(),
      .instanceSize    = sizeof(TransferBuffer),
      .interfaceSize   = sizeof(TransferBufferInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
