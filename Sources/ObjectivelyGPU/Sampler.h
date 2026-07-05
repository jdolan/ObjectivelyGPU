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
 * @brief Sampler wraps an `SDL_GPUSampler`, owning its handle.
 */

typedef struct RenderDevice RenderDevice;
typedef struct Sampler Sampler;
typedef struct SamplerInterface SamplerInterface;

/**
 * @brief An `SDL_GPUSampler` describing texture filter and address modes.
 *
 * Sampler owns its underlying `SDL_GPUSampler` and releases it in `dealloc`, so a
 * Sampler is freed with `release` like any other Objectively object, including
 * ones returned by `RenderDevice`'s `createSampler*` convenience constructors
 * for common filter/address-mode presets.
 *
 * @extends Object
 */
struct Sampler {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  SamplerInterface *interface;

  /**
   * @brief The underlying SDL sampler.
   */
  SDL_GPUSampler *sampler;

  /**
   * @brief The RenderDevice that owns this sampler.
   * @private
   */
  RenderDevice *device;
  
  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The Sampler interface.
 */
struct SamplerInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn Sampler *Sampler::initWithDevice(Sampler *self, RenderDevice *device, const SDL_GPUSamplerCreateInfo *info)
   * @brief Initializes this Sampler, creating its `SDL_GPUSampler`.
   * @details This is the designated initializer.
   * @param self The Sampler.
   * @param device The RenderDevice used to create and release the sampler. Retained.
   * @param info Sampler creation parameters (filters, mip mode, address modes, anisotropy, etc.).
   * @return The initialized Sampler, or `NULL` on failure.
   * @memberof Sampler
   */
  Sampler *(*initWithDevice)(Sampler *self, RenderDevice *device, const SDL_GPUSamplerCreateInfo *info);
};

/**
 * @fn Class *Sampler::_Sampler(void)
 * @brief The Sampler archetype.
 * @return The Sampler Class.
 * @memberof Sampler
 */
OBJECTIVELYGPU_EXPORT Class *_Sampler(void);
