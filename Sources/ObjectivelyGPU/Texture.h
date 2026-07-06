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
#include <SDL3/SDL_surface.h>

#include <Objectively/Object.h>

#include <ObjectivelyGPU/Types.h>

/**
 * @file
 * @brief Texture wraps an `SDL_GPUTexture`, owning its handle and metadata.
 */

typedef struct RenderDevice RenderDevice;
typedef struct Texture Texture;
typedef struct TextureInterface TextureInterface;

/**
 * @brief An `SDL_GPUTexture` and the metadata describing it.
 *
 * Texture owns its underlying `SDL_GPUTexture` and releases it in `dealloc`,
 * so a Texture is freed with `release` like any other Objectively object —
 * there is no separate device-level release call:
 *
 * @code
 *   Texture *texture = $(renderDevice, createTexture, &info, pixels);
 *   // ... use texture->texture, texture->size, texture->format ...
 *   release(texture);
 * @endcode
 *
 * The metadata fields are populated at initialization from the
 * `SDL_GPUTextureCreateInfo` and are immutable thereafter.
 *
 * @extends Object
 */
struct Texture {

  /**
   * @brief The superclass.
   */
  Object object;

  /**
   * @brief The interface.
   * @protected
   */
  TextureInterface *interface;

  /**
   * @brief The underlying SDL texture.
   */
  SDL_GPUTexture *texture;

  /**
   * @brief The pixel format.
   */
  SDL_GPUTextureFormat format;

  /**
   * @brief The texture dimensions in pixels.
   */
  SDL_Size size;

  /**
   * @brief The texture type (2D, cube, 3D, array).
   */
  SDL_GPUTextureType type;

  /**
   * @brief The usage flags the texture was created with.
   */
  SDL_GPUTextureUsageFlags usage;

  /**
   * @brief The depth (3D) or layer count (array/cube), or 1.
   */
  Uint32 layerCountOrDepth;

  /**
   * @brief The number of mip levels.
   */
  Uint32 numLevels;

  /**
   * @brief The MSAA sample count.
   */
  SDL_GPUSampleCount sampleCount;

  /**
   * @brief The RenderDevice that owns this texture.
   * @private
   */
  RenderDevice *device;
  
  /**
   * @brief User data.
   */
  ident data;
};

/**
 * @brief The Texture interface.
 */
struct TextureInterface {

  /**
   * @brief The superclass interface.
   */
  ObjectInterface objectInterface;

  /**
   * @fn Texture *Texture::initWithDevice(Texture *self, RenderDevice *device, const SDL_GPUTextureCreateInfo *info, const void *pixels)
   * @brief Initializes this Texture, creating its `SDL_GPUTexture` and optionally uploading pixel data.
   * @details This is the designated initializer. When @p pixels is non-NULL, a
   *   temporary upload transfer buffer is allocated, @p pixels is copied into it,
   *   a copy pass is recorded and submitted, so the texture is GPU-resident before
   *   returning. Only mip level 0 is uploaded. The upload size is computed with
   *   `SDL_CalculateGPUTextureFormatSize`, so block-compressed formats (BCn, ASTC, …)
   *   are handled correctly; @p pixels must be tightly packed for @p info's format.
   * @param self The Texture.
   * @param device The RenderDevice used to create and release the texture. Retained.
   * @param info Texture creation parameters (format, type, dimensions, usage, etc.).
   * @param pixels Initial tightly-packed pixel data to upload, or `NULL` to leave the texture uninitialised.
   * @return The initialized Texture, or `NULL` on failure.
   * @memberof Texture
   */
  Texture *(*initWithDevice)(Texture *self, RenderDevice *device, const SDL_GPUTextureCreateInfo *info, const void *pixels);

  /**
   * @fn Texture *Texture::initWithSurface(Texture *self, RenderDevice *device, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage, bool generateMipmaps)
   * @brief Initializes this Texture from an `SDL_Surface`, uploading its pixels immediately.
   * @details Converts @p surface to `SDL_PIXELFORMAT_RGBA32` if needed, derives the
   *   dimensions from the surface, and uploads as `SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM`.
   *   The surface's row `pitch` is respected, so padded rows (alignment, or a sub-surface
   *   of an atlas) upload correctly. The surface is neither modified nor freed.
   * @param self The Texture.
   * @param device The RenderDevice used to create and release the texture. Retained.
   * @param surface The source surface. Must not be NULL.
   * @param usage Texture usage flags (e.g. `SDL_GPU_TEXTUREUSAGE_SAMPLER`).
   * @param mipmaps If true, a full mip chain is allocated (level count derived from the
   *   surface's smaller dimension) and generated after the base level upload; this adds
   *   `SDL_GPU_TEXTUREUSAGE_COLOR_TARGET` to @p usage, since mipmap generation is a blit.
   *   If false, only mip level 0 exists, matching prior behavior.
   * @return The initialized Texture, or `NULL` on failure.
   * @memberof Texture
   */
  Texture *(*initWithSurface)(Texture *self, RenderDevice *device, SDL_Surface *surface, SDL_GPUTextureUsageFlags usage, bool generateMipmaps);

  /**
   * @fn void Texture::setName(Texture *self, const char *name)
   * @brief Assigns a debug label to this texture, visible in GPU capture tools.
   * @param self The Texture.
   * @param name A null-terminated debug name string.
   * @memberof Texture
   */
  void (*setName)(Texture *self, const char *name);

  /**
   * @fn void *Texture::downloadPixels(const Texture *self)
   * @brief Downloads mip level 0, layer 0 of this texture to a newly allocated buffer.
   * @details Blocks until the download completes (records a copy pass, submits it with
   *   a fence, and waits on that fence), so this is not suited to per-frame use. The
   *   returned buffer is tightly packed in @p self's native `format` and sized per
   *   `SDL_CalculateGPUTextureFormatSize`; the caller is responsible for interpreting
   *   the pixel format and freeing the buffer with `free`.
   * @param self The Texture.
   * @return A newly allocated buffer of downloaded pixel data. GPU_Asserts on failure.
   * @memberof Texture
   */
  void *(*downloadPixels)(const Texture *self);
};

/**
 * @fn Class *Texture::_Texture(void)
 * @brief The Texture archetype.
 * @return The Texture Class.
 * @memberof Texture
 */
OBJECTIVELYGPU_EXPORT Class *_Texture(void);
