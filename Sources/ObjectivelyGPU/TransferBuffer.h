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

#pragma once

#include <SDL3/SDL_gpu.h>

#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

typedef struct RenderDevice RenderDevice;
typedef struct TransferBuffer TransferBuffer;
typedef struct TransferBufferInterface TransferBufferInterface;

/**
 * @brief A CPU-accessible `SDL_GPUTransferBuffer` for staging data to or from the GPU.
 *
 * TransferBuffer owns its underlying `SDL_GPUTransferBuffer` and releases it in
 * `dealloc`, so a TransferBuffer is freed with `release` like any other Objectively
 * object. Its `buffer` field is the raw handle to embed in an
 * `SDL_GPUTransferBufferLocation` or `SDL_GPUTextureTransferInfo` when calling
 * `CopyPass` upload/download methods:
 *
 * @code
 *   TransferBuffer *tbuf = $(renderDevice, createTransferBuffer,
 *     &(SDL_GPUTransferBufferCreateInfo) { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD, .size = size });
 *
 *   $(copyPass, downloadBuffer, &src, &(SDL_GPUTransferBufferLocation) { .transfer_buffer = tbuf->buffer });
 *
 *   const void *data = $(tbuf, map, false);
 *   ...
 *   $(tbuf, unmap);
 *   release(tbuf);
 * @endcode
 *
 * @extends Object
 */
struct TransferBuffer {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  TransferBufferInterface *interface;

  /**
   * @brief The underlying SDL transfer buffer.
   */
  SDL_GPUTransferBuffer *buffer;

  /**
   * @brief The size of this transfer buffer, in bytes.
   */
  Uint32 size;

  /**
   * @brief The RenderDevice that owns this transfer buffer.
   * @private
   */
  RenderDevice *device;
};

/**
 * @brief The TransferBuffer interface.
 */
struct TransferBufferInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn TransferBuffer *TransferBuffer::initWithDevice(TransferBuffer *self, RenderDevice *device, const SDL_GPUTransferBufferCreateInfo *info)
   * @brief Initializes this TransferBuffer, creating its `SDL_GPUTransferBuffer`.
   * @details This is the designated initializer.
   * @param self The TransferBuffer.
   * @param device The RenderDevice used to create, map, and release the transfer buffer. Retained.
   * @param info Transfer buffer creation parameters (usage: upload or download, size).
   * @return The initialized TransferBuffer. GPU_Asserts on failure.
   * @memberof TransferBuffer
   */
  TransferBuffer *(*initWithDevice)(TransferBuffer *self, RenderDevice *device, const SDL_GPUTransferBufferCreateInfo *info);

  /**
   * @fn void *TransferBuffer::map(const TransferBuffer *self, bool cycle)
   * @brief Maps this transfer buffer into CPU-accessible memory for reading or writing.
   * @details Pass `cycle = true` to let the driver use a fresh buffer allocation
   *   (avoiding a pipeline stall) when the buffer is already in use by the GPU.
   *   Call `unmap` when done; the mapping must not be used after that.
   * @param self The TransferBuffer.
   * @param cycle If `true`, the driver may cycle to a new backing allocation.
   * @return A CPU pointer to the mapped region. @c GPU_Asserts on failure.
   * @memberof TransferBuffer
   */
  void *(*map)(const TransferBuffer *self, bool cycle);

  /**
   * @fn void TransferBuffer::unmap(const TransferBuffer *self)
   * @brief Unmaps this transfer buffer previously mapped with `map`.
   * @details The CPU pointer returned by `map` must not be accessed after this call.
   * @param self The TransferBuffer.
   * @memberof TransferBuffer
   */
  void (*unmap)(const TransferBuffer *self);
};

/**
 * @fn Class *TransferBuffer::_TransferBuffer(void)
 * @brief The TransferBuffer archetype.
 * @return The TransferBuffer Class.
 * @memberof TransferBuffer
 */
OBJECTIVELYGPU_EXPORT Class *_TransferBuffer(void);
