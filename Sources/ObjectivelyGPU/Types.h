#pragma once

#include <Objectively/Types.h>

#ifndef OBJECTIVELYGPU_EXPORT
 #define OBJECTIVELYGPU_EXPORT extern
#endif

typedef struct SDL_Size SDL_Size;

struct SDL_Size {
  int w, h;
};

#define MakeSize(w, h) (SDL_Size) { (w), (h) }
