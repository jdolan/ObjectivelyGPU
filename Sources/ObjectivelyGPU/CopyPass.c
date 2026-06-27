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
#include <string.h>

#include "CopyPass.h"

#define _Class _CopyPass

#pragma mark - Object

/**
 * @see Object::dealloc(Object *)
 */
static void dealloc(Object *self) {

  CopyPass *this = (CopyPass *) self;
  if (this->pass) {
    SDL_EndGPUCopyPass(this->pass);
  }

  super(Object, self, dealloc);
}

#pragma mark - CopyPass

/**
 * @fn void CopyPass::copyBufferToBuffer(const CopyPass *self, const SDL_GPUBufferLocation *src, const SDL_GPUBufferLocation *dst, Uint32 size, bool cycle)
 * @memberof CopyPass
 */
static void copyBufferToBuffer(const CopyPass *self, const SDL_GPUBufferLocation *src, const SDL_GPUBufferLocation *dst, Uint32 size, bool cycle) {
  SDL_CopyGPUBufferToBuffer(self->pass, src, dst, size, cycle);
}

/**
 * @fn void CopyPass::copyTextureToTexture(const CopyPass *self, const SDL_GPUTextureLocation *src, const SDL_GPUTextureLocation *dst, Uint32 w, Uint32 h, Uint32 d, bool cycle)
 * @memberof CopyPass
 */
static void copyTextureToTexture(const CopyPass *self, const SDL_GPUTextureLocation *src, const SDL_GPUTextureLocation *dst, Uint32 w, Uint32 h, Uint32 d, bool cycle) {
  SDL_CopyGPUTextureToTexture(self->pass, src, dst, w, h, d, cycle);
}

/**
 * @fn void CopyPass::downloadBuffer(const CopyPass *self, const SDL_GPUBufferRegion *src, const SDL_GPUTransferBufferLocation *dst)
 * @memberof CopyPass
 */
static void downloadBuffer(const CopyPass *self, const SDL_GPUBufferRegion *src, const SDL_GPUTransferBufferLocation *dst) {
  SDL_DownloadFromGPUBuffer(self->pass, src, dst);
}

/**
 * @fn void CopyPass::downloadTexture(const CopyPass *self, const SDL_GPUTextureRegion *src, const SDL_GPUTextureTransferInfo *dst)
 * @memberof CopyPass
 */
static void downloadTexture(const CopyPass *self, const SDL_GPUTextureRegion *src, const SDL_GPUTextureTransferInfo *dst) {
  SDL_DownloadFromGPUTexture(self->pass, src, dst);
}

/**
 * @fn CopyPass *CopyPass::init(CopyPass *self, SDL_GPUCopyPass *pass, SDL_GPUDevice *device)
 * @memberof CopyPass
 */
static CopyPass *init(CopyPass *self, SDL_GPUCopyPass *pass, SDL_GPUDevice *device) {

  self = (CopyPass *) super(Object, self, init);
  if (self) {
    self->pass = pass;
    assert(self->pass);
    self->device = device;
    assert(self->device);
  }

  return self;
}

/**
 * @fn void CopyPass::uploadBuffer(const CopyPass *self, const SDL_GPUTransferBufferLocation *src, const SDL_GPUBufferRegion *dst, bool cycle)
 * @memberof CopyPass
 */
static void uploadBuffer(const CopyPass *self, const SDL_GPUTransferBufferLocation *src, const SDL_GPUBufferRegion *dst, bool cycle) {
  SDL_UploadToGPUBuffer(self->pass, src, dst, cycle);
}

/**
 * @fn void CopyPass::uploadData(const CopyPass *self, SDL_GPUBuffer *dst, const void *data, Uint32 size, Uint32 offset, bool cycle)
 * @memberof CopyPass
 */
static void uploadData(const CopyPass *self, SDL_GPUBuffer *dst, const void *data, Uint32 size, Uint32 offset, bool cycle) {
  assert(self);
  assert(dst);
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

  SDL_UploadToGPUBuffer(self->pass,
    &(SDL_GPUTransferBufferLocation) { .transfer_buffer = tbuf },
    &(SDL_GPUBufferRegion) { .buffer = dst, .offset = offset, .size = size },
    cycle);

  SDL_ReleaseGPUTransferBuffer(self->device, tbuf);
}

/**
 * @fn void CopyPass::uploadTexture(const CopyPass *self, const SDL_GPUTextureTransferInfo *src, const SDL_GPUTextureRegion *dst, bool cycle)
 * @memberof CopyPass
 */
static void uploadTexture(const CopyPass *self, const SDL_GPUTextureTransferInfo *src, const SDL_GPUTextureRegion *dst, bool cycle) {
  SDL_UploadToGPUTexture(self->pass, src, dst, cycle);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

  ((ObjectInterface *) clazz->interface)->dealloc = dealloc;

  ((CopyPassInterface *) clazz->interface)->copyBufferToBuffer = copyBufferToBuffer;
  ((CopyPassInterface *) clazz->interface)->copyTextureToTexture = copyTextureToTexture;
  ((CopyPassInterface *) clazz->interface)->downloadBuffer = downloadBuffer;
  ((CopyPassInterface *) clazz->interface)->downloadTexture = downloadTexture;
  ((CopyPassInterface *) clazz->interface)->init = init;
  ((CopyPassInterface *) clazz->interface)->uploadBuffer = uploadBuffer;
  ((CopyPassInterface *) clazz->interface)->uploadData = uploadData;
  ((CopyPassInterface *) clazz->interface)->uploadTexture = uploadTexture;
}

/**
 * @fn Class *CopyPass::_CopyPass(void)
 * @memberof CopyPass
 */
Class *_CopyPass(void) {
  static Class *clazz;
  static Once once;

  do_once(&once, {
    clazz = _initialize(&(const ClassDef) {
      .name            = "CopyPass",
      .superclass      = _Object(),
      .instanceSize    = sizeof(CopyPass),
      .interfaceOffset = offsetof(CopyPass, interface),
      .interfaceSize   = sizeof(CopyPassInterface),
      .initialize      = initialize,
    });
  });

  return clazz;
}

#undef _Class
