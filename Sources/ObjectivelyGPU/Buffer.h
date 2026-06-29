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

#pragma once

#include <SDL3/SDL_gpu.h>

#include <Objectively/Data.h>
#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief Buffer wraps an `SDL_GPUBuffer`, owning its handle and metadata.
 */

typedef struct RenderDevice RenderDevice;
typedef struct Buffer Buffer;
typedef struct BufferInterface BufferInterface;

/**
 * @brief An `SDL_GPUBuffer` (vertex, index, indirect, storage, etc.) and its metadata.
 *
 * Buffer owns its underlying `SDL_GPUBuffer` and releases it in `dealloc`, so a
 * Buffer is freed with `release` like any other Objectively object — there is no
 * separate device-level release call:
 *
 * @code
 *   Buffer *vertices = $(renderDevice, createBufferWithConstMem,
 *     SDL_GPU_BUFFERUSAGE_VERTEX, data, sizeof(data));
 *   $(pass, bindVertexBuffers, 0, &(SDL_GPUBufferBinding) { .buffer = vertices->buffer }, 1);
 *   release(vertices);
 * @endcode
 *
 * Use `upload` to re-populate a dynamic buffer (sprites, UI, particles) each frame.
 *
 * @extends Object
 */
struct Buffer {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  BufferInterface *interface;

  /**
   * @brief The underlying SDL buffer.
   */
  SDL_GPUBuffer *buffer;

  /**
   * @brief The usage flags the buffer was created with.
   */
  SDL_GPUBufferUsageFlags usage;

  /**
   * @brief The size of the buffer, in bytes.
   */
  Uint32 size;

  /**
   * @brief The RenderDevice that owns this buffer.
   * @private
   */
  RenderDevice *device;
};

/**
 * @brief The Buffer interface.
 */
struct BufferInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn Buffer *Buffer::initWithConstMem(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size)
   * @brief Initializes this Buffer and uploads @p mem in a single call.
   * @details Convenience initializer that creates a buffer sized to @p size via
   *   `initWithDevice`, then uploads @p mem with `upload`. The returned buffer is
   *   immediately GPU-resident.
   * @param self The Buffer.
   * @param device The RenderDevice used to create and release the buffer. Retained.
   * @param usage Buffer usage flags (e.g. `SDL_GPU_BUFFERUSAGE_VERTEX`).
   * @param mem CPU pointer to the source data. Must not be NULL.
   * @param size Number of bytes to allocate and upload.
   * @return The initialized Buffer, or `NULL` on failure.
   * @memberof Buffer
   */
  Buffer *(*initWithConstMem)(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const void *mem, Uint32 size);

  /**
   * @fn Buffer *Buffer::initWithData(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const Data *data)
   * @brief Initializes this Buffer and uploads the contents of an Objectively `Data`.
   * @details Convenience initializer over `initWithConstMem` for data loaded via the
   *   Objectively Resource system.
   * @param self The Buffer.
   * @param device The RenderDevice used to create and release the buffer. Retained.
   * @param usage Buffer usage flags (e.g. `SDL_GPU_BUFFERUSAGE_VERTEX`).
   * @param data Source data object. Must not be NULL.
   * @return The initialized Buffer, or `NULL` on failure.
   * @memberof Buffer
   */
  Buffer *(*initWithData)(Buffer *self, RenderDevice *device, SDL_GPUBufferUsageFlags usage, const Data *data);

  /**
   * @fn Buffer *Buffer::initWithDevice(Buffer *self, RenderDevice *device, const SDL_GPUBufferCreateInfo *info)
   * @brief Initializes this Buffer, creating its `SDL_GPUBuffer`.
   * @details This is the designated initializer. The buffer's contents are
   *   uninitialised; use `upload` to populate it.
   * @param self The Buffer.
   * @param device The RenderDevice used to create and release the buffer. Retained.
   * @param info Buffer creation parameters (usage flags, size).
   * @return The initialized Buffer, or `NULL` on failure.
   * @memberof Buffer
   */
  Buffer *(*initWithDevice)(Buffer *self, RenderDevice *device, const SDL_GPUBufferCreateInfo *info);

  /**
   * @fn void Buffer::setName(Buffer *self, const char *name)
   * @brief Assigns a debug label to this buffer, visible in GPU capture tools.
   * @param self The Buffer.
   * @param name A null-terminated debug name string.
   * @memberof Buffer
   */
  void (*setName)(Buffer *self, const char *name);

  /**
   * @fn void Buffer::upload(Buffer *self, const void *data, Uint32 size, Uint32 offset, bool cycle)
   * @brief Uploads raw CPU data into this buffer.
   * @details Acquires a command buffer, creates a temporary transfer buffer, records a
   *   copy pass, submits, and releases the transfer buffer. Use this for dynamic buffers
   *   re-uploaded each frame. To batch multiple uploads into one command buffer, use
   *   `CopyPass::uploadData` instead.
   * @param self The Buffer.
   * @param data CPU pointer to the source data. Must not be NULL.
   * @param size Number of bytes to upload.
   * @param offset Byte offset into this buffer where the data is written.
   * @param cycle When true, the buffer is cycled to avoid pipeline stalls.
   * @memberof Buffer
   */
  void (*upload)(Buffer *self, const void *data, Uint32 size, Uint32 offset, bool cycle);
};

/**
 * @fn Class *Buffer::_Buffer(void)
 * @brief The Buffer archetype.
 * @return The Buffer Class.
 * @memberof Buffer
 */
OBJECTIVELYGPU_EXPORT Class *_Buffer(void);
