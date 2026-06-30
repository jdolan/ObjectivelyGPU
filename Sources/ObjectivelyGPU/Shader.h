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
#include <Objectively/Resource.h>

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
   * @brief The interface.
   * @protected
   */
  ShaderInterface *interface;

  /**
   * @brief The underlying SDL shader.
   */
  SDL_GPUShader *shader;

  /**
   * @brief The shader stage (vertex or fragment).
   */
  SDL_GPUShaderStage stage;

  /**
   * @brief The RenderDevice that owns this shader.
   * @private
   */
  RenderDevice *device;
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
   * @details This is the designated initializer. All fields of @p info, including
   *   `code`, `code_size`, and `format`, must be set by the caller. Prefer
   *   `initWithResourceName` to load a compiled blob from the Resource system with
   *   automatic format selection.
   * @param self The Shader.
   * @param device The RenderDevice used to create and release the shader. Retained.
   * @param info Shader creation parameters with all fields populated.
   * @return The initialized Shader, or `NULL` on failure.
   * @memberof Shader
   */
  Shader *(*initWithDevice)(Shader *self, RenderDevice *device, const SDL_GPUShaderCreateInfo *info);

  /**
   * @fn Shader *Shader::initWithResource(Shader *self, RenderDevice *device, const Resource *resource, SDL_GPUShaderFormat format, const SDL_GPUShaderCreateInfo *info)
   * @brief Initializes this Shader from an already-loaded compiled shader blob.
   * @details Uses @p resource's data as the shader `code` for the given @p format.
   *   `entrypoint` defaults to `main0` (MSL) or `main` when not set; the `code`,
   *   `code_size`, and `format` fields of @p info are ignored. `initWithResourceName`
   *   resolves a name to a resource and format, then delegates here.
   * @param self The Shader.
   * @param device The RenderDevice used to create and release the shader. Retained.
   * @param resource The loaded shader blob; must have non-empty data.
   * @param format The shader format of @p resource's data.
   * @param info Shader creation parameters; `code`, `code_size`, and `format` are ignored.
   * @return The initialized Shader, or `NULL` on failure.
   * @memberof Shader
   */
  Shader *(*initWithResource)(Shader *self, RenderDevice *device, const Resource *resource, SDL_GPUShaderFormat format, const SDL_GPUShaderCreateInfo *info);

  /**
   * @fn Shader *Shader::initWithResourceName(Shader *self, RenderDevice *device, const char *name, const SDL_GPUShaderCreateInfo *info)
   * @brief Initializes this Shader from a compiled shader blob loaded via the Resource system.
   * @details Appends the platform-appropriate extension to @p name and resolves it via
   *   Objectively's ResourceProvider chain:
   *   - Metal (macOS/iOS): `.metal`
   *   - Vulkan (Linux/Android): `.spv`
   *   - D3D12 (Windows): `.dxil`
   *   The first supported, resolvable, non-empty blob is selected and passed to
   *   `initWithResource` with its format. The caller fills in @c stage and binding counts
   *   in @p info; `code`, `code_size`, `format`, and a default `entrypoint` are supplied.
   *   Shader blobs are produced offline by @c sdl-shadercross.
   * @param self The Shader.
   * @param device The RenderDevice used to create and release the shader. Retained.
   * @param name Shader base name without extension, e.g. @c "shaders/Renderer.vert".
   * @param info Shader creation parameters; `code`, `code_size`, and `format` are ignored.
   * @return The initialized Shader. GPU_Asserts if no supported blob is found.
   * @memberof Shader
   */
  Shader *(*initWithResourceName)(Shader *self, RenderDevice *device, const char *name, const SDL_GPUShaderCreateInfo *info);
};

/**
 * @fn Class *Shader::_Shader(void)
 * @brief The Shader archetype.
 * @return The Shader Class.
 * @memberof Shader
 */
OBJECTIVELYGPU_EXPORT Class *_Shader(void);
