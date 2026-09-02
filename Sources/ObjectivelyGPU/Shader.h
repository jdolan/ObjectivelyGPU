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
 * @brief Shader wraps an `SDL_GPUShader`, owning its handle.
 */

typedef struct RenderDevice RenderDevice;
typedef struct Shader Shader;
typedef struct ShaderInterface ShaderInterface;

/**
 * @brief An `SDL_GPUShader`: one compiled programmable stage of a graphics pipeline.
 *
 * Shader owns its underlying `SDL_GPUShader` and releases it in `dealloc`, so a
 * shader is freed with `release` like any other Objectively object. A shader need
 * only outlive the `GraphicsPipeline` creation that consumes it, after which it
 * may be released. Pass the `shader` handle into the pipeline's create info.
 *
 * @extends Object
 */
struct Shader {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface type.
   * @protected
   */
  ShaderInterface *interface[0];

  /**
   * @brief The RenderDevice that owns this shader.
   * @remarks Weak: a resource MUST NOT outlive the device that owns it.
   * @private
   */
  RenderDevice *device;
  
  /**
   * @brief The underlying SDL shader.
   */
  SDL_GPUShader *shader;

  /**
   * @brief The shader format.
   */
  SDL_GPUShaderFormat format;
  
  /**
   * @brief The shader stage (vertex or fragment).
   */
  SDL_GPUShaderStage stage;
  
  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The Shader interface.
 */
struct ShaderInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn Shader *Shader::initWithDevice(Shader *self, RenderDevice *device, const SDL_GPUShaderCreateInfo *info)
   * @brief Initializes this Shader from a fully-populated `SDL_GPUShaderCreateInfo`.
   * @details The designated (and only) initializer: a thin wrapper over
   *   `SDL_CreateGPUShader`. All fields of @p info, including `code`, `code_size`,
   *   `format`, and `entrypoint`, must be set by the caller. Prefer
   *   `RenderDevice::loadShader`, which resolves a compiled blob from the Resource
   *   system, selects the device-appropriate format, and fills these fields in.
   * @param self The Shader.
   * @param device The RenderDevice used to create and release the shader. Retained.
   * @param info The Shader creation parameters with all fields populated.
   * @return The initialized Shader, or `NULL` on failure.
   * @memberof Shader
   */
  Shader *(*initWithDevice)(Shader *self, RenderDevice *device, const SDL_GPUShaderCreateInfo *info);
};

/**
 * @fn Class *Shader::_Shader(void)
 * @brief The Shader archetype.
 * @return The Shader Class.
 * @memberof Shader
 */
OBJECTIVELYGPU_EXPORT Class *_Shader(void);
