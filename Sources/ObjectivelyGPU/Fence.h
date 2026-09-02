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
typedef struct Fence Fence;
typedef struct FenceInterface FenceInterface;

/**
 * @brief An `SDL_GPUFence`, signaled once a submitted CommandBuffer's GPU work completes.
 *
 * Fence owns its underlying `SDL_GPUFence` and releases it in `dealloc`, so a Fence is
 * freed with `release` like any other Objectively object -- e.g. by storing pending
 * fences in an `Array`, which releases each element automatically as it's removed:
 *
 * @code
 *   Fence *fence = $(commands, submitAndFence);
 *   $(pending, addObject, fence);
 *   release(fence);
 *   ...
 *   if ($((Fence *) $(pending, objectAtIndex, 0), query)) {
 *     $(pending, removeObjectAtIndex, 0); // releases the Fence
 *   }
 * @endcode
 *
 * @extends Object
 */
struct Fence {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface type.
   * @protected
   */
  FenceInterface *interface[0];

  /**
   * @brief The underlying SDL fence.
   */
  SDL_GPUFence *fence;

  /**
   * @brief The RenderDevice that owns this fence.
   * @remarks Weak: a resource MUST NOT outlive the device that owns it.
   * @private
   */
  RenderDevice *device;
};

/**
 * @brief The Fence interface.
 */
struct FenceInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn Fence *Fence::initWithDevice(Fence *self, RenderDevice *device, SDL_GPUFence *fence)
   * @brief Initializes this Fence, wrapping an `SDL_GPUFence` returned by `CommandBuffer::submitAndFence`.
   * @details This is the designated initializer.
   * @param self The Fence.
   * @param device The RenderDevice used to query, wait on, and release the fence. Retained.
   * @param fence The `SDL_GPUFence` to wrap. Owned by the returned Fence.
   * @return The initialized Fence, or `NULL` on error.
   * @memberof Fence
   */
  Fence *(*initWithDevice)(Fence *self, RenderDevice *device, SDL_GPUFence *fence);

  /**
   * @fn bool Fence::query(const Fence *self)
   * @brief Non-blocking query of whether this fence has been signaled.
   * @param self The Fence.
   * @return `true` if signaled (the GPU work it guards has completed); `false` otherwise.
   * @memberof Fence
   */
  bool (*query)(const Fence *self);

  /**
   * @fn bool Fence::wait(const Fence *self)
   * @brief Blocks the calling thread until this fence is signaled.
   * @param self The Fence.
   * @return `true` on success, `false` on error.
   * @memberof Fence
   */
  bool (*wait)(const Fence *self);
};

/**
 * @fn Class *Fence::_Fence(void)
 * @brief The Fence archetype.
 * @return The Fence Class.
 * @memberof Fence
 */
OBJECTIVELYGPU_EXPORT Class *_Fence(void);
