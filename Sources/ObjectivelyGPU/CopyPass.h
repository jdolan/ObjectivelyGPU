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

#pragma once

#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief CopyPass wraps an `SDL_GPUCopyPass` for uploading and copying GPU resources.
 */

typedef struct CommandBuffer CommandBuffer;
typedef struct CopyPass CopyPass;
typedef struct CopyPassInterface CopyPassInterface;

/**
 * @brief A scoped copy pass for uploading CPU data and copying GPU resources.
 *
 * Obtain a CopyPass from `CommandBuffer::beginCopyPass`. When all transfers
 * are complete, release the pass — `dealloc` calls `SDL_EndGPUCopyPass`
 * automatically so no explicit `end` is required:
 *
 * @code
 *   CopyPass *copy = $(commands, beginCopyPass);
 *   $(copy, uploadBuffer, &src, &dst, false);
 *   release(copy);  // ends the pass
 * @endcode
 *
 * @extends Object
 */
struct CopyPass {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  CopyPassInterface *interface;

  /**
   * @brief The CommandBuffer this pass was begun from.
   * @private
   */
  CommandBuffer *commands;

  /**
   * @brief The underlying SDL copy pass.
   * @details Set to NULL by `dealloc` after `SDL_EndGPUCopyPass` is called.
   * @private
   */
  SDL_GPUCopyPass *pass;
};

/**
 * @brief The CopyPass interface.
 */
struct CopyPassInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn void CopyPass::copyBufferToBuffer(const CopyPass *self, const SDL_GPUBufferLocation *src, const SDL_GPUBufferLocation *dst, Uint32 size, bool cycle)
   * @brief Copies a region of one GPU buffer to another.
   * @memberof CopyPass
   */
  void (*copyBufferToBuffer)(const CopyPass *self, const SDL_GPUBufferLocation *src, const SDL_GPUBufferLocation *dst, Uint32 size, bool cycle);

  /**
   * @fn void CopyPass::copyTextureToTexture(const CopyPass *self, const SDL_GPUTextureLocation *src, const SDL_GPUTextureLocation *dst, Uint32 w, Uint32 h, Uint32 d, bool cycle)
   * @brief Copies a region of one GPU texture to another.
   * @memberof CopyPass
   */
  void (*copyTextureToTexture)(const CopyPass *self, const SDL_GPUTextureLocation *src, const SDL_GPUTextureLocation *dst, Uint32 w, Uint32 h, Uint32 d, bool cycle);

  /**
   * @fn void CopyPass::downloadBuffer(const CopyPass *self, const SDL_GPUBufferRegion *src, const SDL_GPUTransferBufferLocation *dst)
   * @brief Downloads a GPU buffer region to a CPU-accessible transfer buffer.
   * @memberof CopyPass
   */
  void (*downloadBuffer)(const CopyPass *self, const SDL_GPUBufferRegion *src, const SDL_GPUTransferBufferLocation *dst);

  /**
   * @fn void CopyPass::downloadTexture(const CopyPass *self, const SDL_GPUTextureRegion *src, const SDL_GPUTextureTransferInfo *dst)
   * @brief Downloads a GPU texture region to a CPU-accessible transfer buffer.
   * @memberof CopyPass
   */
  void (*downloadTexture)(const CopyPass *self, const SDL_GPUTextureRegion *src, const SDL_GPUTextureTransferInfo *dst);

  /**
   * @fn CopyPass *CopyPass::init(CopyPass *self, CommandBuffer *commands, SDL_GPUCopyPass *pass)
   * @brief Initializes this CopyPass wrapping the given SDL copy pass.
   * @param self The CopyPass.
   * @param commands The CommandBuffer that created this pass.
   * @param pass The SDL copy pass to wrap. Must not be NULL.
   * @return The initialized CopyPass, or NULL on failure.
   * @memberof CopyPass
   */
  CopyPass *(*init)(CopyPass *self, CommandBuffer *commands, SDL_GPUCopyPass *pass);

  /**
   * @fn void CopyPass::uploadBuffer(const CopyPass *self, const SDL_GPUTransferBufferLocation *src, const SDL_GPUBufferRegion *dst, bool cycle)
   * @brief Uploads data from a CPU transfer buffer to a GPU buffer.
   * @param self The CopyPass.
   * @param src Source location in the CPU-side transfer buffer.
   * @param dst Destination region in the GPU buffer.
   * @param cycle When true, the GPU buffer is cycled to avoid pipeline stalls.
   * @memberof CopyPass
   */
  void (*uploadBuffer)(const CopyPass *self, const SDL_GPUTransferBufferLocation *src, const SDL_GPUBufferRegion *dst, bool cycle);

  /**
   * @fn void CopyPass::uploadData(const CopyPass *self, SDL_GPUBuffer *dst, const void *data, Uint32 size, Uint32 offset, bool cycle)
   * @brief Uploads raw CPU data to a GPU buffer, managing the transfer buffer internally.
   * @details Allocates a temporary transfer buffer, copies @p data into it, records
   *   the upload, and releases the transfer buffer — all within this call. Use this
   *   to batch multiple buffer uploads into a single copy pass without manual transfer
   *   buffer management.
   * @param self The CopyPass.
   * @param dst The GPU buffer to upload into.
   * @param data CPU pointer to the source data.
   * @param size Number of bytes to upload.
   * @param offset Byte offset into @p dst where the data is written.
   * @param cycle When true, the GPU buffer is cycled to avoid pipeline stalls.
   * @memberof CopyPass
   */
  void (*uploadData)(const CopyPass *self, SDL_GPUBuffer *dst, const void *data, Uint32 size, Uint32 offset, bool cycle);

  /**
   * @fn void CopyPass::uploadTexture(const CopyPass *self, const SDL_GPUTextureTransferInfo *src, const SDL_GPUTextureRegion *dst, bool cycle)
   * @brief Uploads data from a CPU transfer buffer to a GPU texture.
   * @param self The CopyPass.
   * @param src Source transfer info describing the CPU-side data layout.
   * @param dst Destination region in the GPU texture.
   * @param cycle When true, the GPU texture is cycled to avoid pipeline stalls.
   * @memberof CopyPass
   */
  void (*uploadTexture)(const CopyPass *self, const SDL_GPUTextureTransferInfo *src, const SDL_GPUTextureRegion *dst, bool cycle);
};

/**
 * @fn Class *CopyPass::_CopyPass(void)
 * @brief The CopyPass archetype.
 * @return The CopyPass Class.
 * @memberof CopyPass
 */
OBJECTIVELYGPU_EXPORT Class *_CopyPass(void);
