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

/**
 * @file
 * @brief QueryPool wraps an `SDL_GPUQueryPool`, owning its handle and metadata.
 * @details The occlusion/timestamp query API is a prototype not yet part of
 *   upstream SDL. The types and methods here are always available, but when
 *   linked against an SDL3 build that does not define `SDL_GPU_OCCLUSION_QUERY`,
 *   this class and the related `RenderPass`/`CopyPass` query methods are inert
 *   no-ops: `initWithDevice` still succeeds (so callers never need to special-case
 *   its absence), but `beginQuery`/`endQuery` do nothing and
 *   `downloadQueryResults` always reports "not occluded". This lets client code
 *   (e.g. Quetoo) unconditionally use occlusion queries as an optimization that
 *   silently disables itself where unsupported.
 */

#ifndef SDL_GPU_OCCLUSION_QUERY

/**
 * @brief Polyfills for the query types this prototype API depends on, so
 *   ObjectivelyGPU's own headers can declare that API unconditionally even
 *   when the linked SDL3 doesn't yet provide it.
 */
typedef struct SDL_GPUQueryPool SDL_GPUQueryPool;

typedef enum SDL_GPUQueryType {
  SDL_GPU_QUERY_TIMESTAMP,
  SDL_GPU_QUERY_BINARY_OCCLUSION,
  SDL_GPU_QUERY_PRECISE_OCCLUSION
} SDL_GPUQueryType;

typedef struct SDL_GPUQueryPoolCreateInfo {
  SDL_GPUQueryType type;
  Uint32 query_count;
  SDL_PropertiesID props;
} SDL_GPUQueryPoolCreateInfo;

#endif // !SDL_GPU_OCCLUSION_QUERY

typedef struct RenderDevice RenderDevice;
typedef struct QueryPool QueryPool;
typedef struct QueryPoolInterface QueryPoolInterface;

/**
 * @brief An `SDL_GPUQueryPool` and its metadata.
 *
 * QueryPool owns its underlying `SDL_GPUQueryPool` and releases it in
 * `dealloc`, so a QueryPool is freed with `release` like any other
 * Objectively object. Begin and end queries against it via
 * `RenderPass::beginQuery` / `RenderPass::endQuery`, and retrieve results via
 * `CopyPass::downloadQueryResults`:
 *
 * @code
 *   QueryPool *queries = $(renderDevice, createQueryPool,
 *     &(SDL_GPUQueryPoolCreateInfo) { .type = SDL_GPU_QUERY_BINARY_OCCLUSION, .query_count = 1 });
 *
 *   $(pass, beginQuery, queries, 0);
 *   $(pass, drawPrimitives, ...);
 *   $(pass, endQuery, queries, 0);
 *
 *   $(copyPass, downloadQueryResults, queries, 0, 1, &destination);
 *   release(queries);
 * @endcode
 *
 * @extends Object
 */
struct QueryPool {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  QueryPoolInterface *interface;

  /**
   * @brief The underlying SDL query pool, or `NULL` if occlusion queries are
   *   unsupported by the linked SDL3 (see @ref QueryPool.h).
   */
  SDL_GPUQueryPool *pool;

  /**
   * @brief The type of query this pool was created for.
   */
  SDL_GPUQueryType type;

  /**
   * @brief The maximum number of queries in this pool.
   */
  Uint32 queryCount;

  /**
   * @brief The RenderDevice that owns this query pool.
   * @private
   */
  RenderDevice *device;

  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The QueryPool interface.
 */
struct QueryPoolInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn QueryPool *QueryPool::initWithDevice(QueryPool *self, RenderDevice *device, const SDL_GPUQueryPoolCreateInfo *info)
   * @brief Initializes this QueryPool, creating its `SDL_GPUQueryPool`.
   * @details This is the designated initializer. Always succeeds, even when
   *   occlusion queries are unsupported by the linked SDL3 -- `self->pool` is
   *   `NULL` in that case, and the pool behaves as an inert no-op.
   * @param self The QueryPool.
   * @param device The RenderDevice used to create and release the query pool. Retained.
   * @param info Query pool creation parameters (query type, query count).
   * @return The initialized QueryPool, or `NULL` on failure.
   * @memberof QueryPool
   */
  QueryPool *(*initWithDevice)(QueryPool *self, RenderDevice *device, const SDL_GPUQueryPoolCreateInfo *info);
};

/**
 * @fn Class *QueryPool::_QueryPool(void)
 * @brief The QueryPool archetype.
 * @return The QueryPool Class.
 * @memberof QueryPool
 */
OBJECTIVELYGPU_EXPORT Class *_QueryPool(void);
