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

#include <stdlib.h>

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>

#include <Objectively/Types.h>

#ifndef OBJECTIVELYGPU_EXPORT
 #define OBJECTIVELYGPU_EXPORT extern
#endif

typedef struct SDL_Size SDL_Size;

/**
 * @brief The `SDL_Size` type.
 */
struct SDL_Size {
  int w, h;
};

/**
 * @brief Creates an `SDL_Point` with the given coordinates.
 */
#define MakePoint(x, y) (SDL_Point) { (x), (y) }

/**
 * @brief Creates an `SDL_Rect` with the given origin and size.
 */
#define MakeRect(x, y, w, h) (SDL_Rect) { (x), (y), (w), (h) }

/**
 * @brief Creates an `SDL_Size` with the given dimensions.
 */
#define MakeSize(w, h) (SDL_Size) { (w), (h) }

/**
 * @brief Asserts that @a cond is true, logging the SDL error and exiting on failure.
 * @details Unlike assert(3), this macro is never compiled out.
 */
#define GPU_Assert(cond, fmt, ...) \
  do { \
    if (!(cond)) { \
      SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "%s::%d::%s: " fmt ": %s", __FILE__, __LINE__, __func__, ## __VA_ARGS__, SDL_GetError()); \
      SDL_TriggerBreakpoint(); \
      exit(EXIT_FAILURE); \
    } \
  } while (0)
