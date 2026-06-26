# ObjectivelyGPU — Copilot Instructions

ObjectivelyGPU is the GPU layer that sits **between** [Objectively](https://github.com/jdolan/Objectively) (GNU C OOP runtime) and [ObjectivelyMVC](https://github.com/jdolan/ObjectivelyMVC) (MVC UI framework). It wraps SDL3's GPU API and provides the rendering backend that ObjectivelyMVC's `Renderer` uses to draw its `View` hierarchy.

---

## Stack Overview

```
ObjectivelyMVC  ── View hierarchy, layout, theming, events
                   └─ Renderer  (accumulates draw calls per frame)
                       └─ RenderDevice (ObjectivelyGPU)
ObjectivelyGPU  ── RenderDevice, CommandBuffer, RenderPass, CopyPass, ComputePass
                   └─ SDL_GPUDevice  (SDL3 GPU API)
Objectively     ── Object, Class, alloc/retain/release, $(), Vector, Array, Resource…
```

---

## Language & Style

- **GNU C** (gcc/clang only). Use `#pragma once` for header guards.
- Every `.c` and `.h` file begins with the project's zlib license header.
- `assert()` all non-trivial pointer arguments at the top of every function.
- Use `GPU_Assert(cond, fmt, ...)` (from `Types.h`) for unrecoverable SDL GPU failures — never `SDL_assert` or bare `assert` for GPU calls.
- All names follow **camelCase** for variables and methods, **PascalCase** for types and classes, `UPPER_SNAKE_CASE` for macros and enum constants.

---

## Objectively OOP Pattern

Every class in this codebase (and all three libraries) follows the same pattern exactly.

### Header (`Foo.h`)

```c
// 1. Forward-declare all types referenced in this file
typedef struct Bar Bar;
typedef struct Foo Foo;
typedef struct FooInterface FooInterface;

/**
 * @brief One-line description of Foo.
 * @extends Bar
 */
struct Foo {
    Bar bar;                   // superclass is always first
    FooInterface *interface;   // @protected
    int publicField;
    SomeType *privateField;    // @private
};

/**
 * @brief The Foo interface (vtable).
 */
struct FooInterface {
    BarInterface barInterface; // superclass interface is always first

    /**
     * @fn ReturnType Foo::methodName(Foo *self, ParamType param)
     * @brief One-line description.
     * @param self The Foo.
     * @param param Description.
     * @return Description.
     * @memberof Foo
     */
    ReturnType (*methodName)(Foo *self, ParamType param);
};

/**
 * @fn Class *Foo::_Foo(void)
 * @brief The Foo archetype.
 * @return The Foo Class.
 * @memberof Foo
 */
OBJECTIVELYGPU_EXPORT Class *_Foo(void);
```

### Implementation (`Foo.c`)

```c
#include "Bar.h"
#include "Foo.h"

#define _Class _Foo

#pragma mark - Object

static void dealloc(Object *self) {
    Foo *this = (Foo *) self;

    release(this->someOwnedObject);  // release all retained fields
    SDL_ReleaseGPUSomething(...);    // free non-refcounted GPU resources

    super(Object, self, dealloc);    // always call super last
}

#pragma mark - Foo

/**
 * @fn ReturnType Foo::methodName(Foo *self, ParamType param)
 * @memberof Foo
 */
static ReturnType methodName(Foo *self, ParamType param) {
    assert(self);
    assert(param);
    // ...
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {
    ((ObjectInterface *) clazz->interface)->dealloc = dealloc;
    ((FooInterface *) clazz->interface)->methodName = methodName;
}

/**
 * @fn Class *Foo::_Foo(void)
 * @memberof Foo
 */
Class *_Foo(void) {
    static Class *clazz;
    static Once once;

    do_once(&once, {
        clazz = _initialize(&(const ClassDef) {
            .name = "Foo",
            .superclass = _Bar(),
            .instanceSize = sizeof(Foo),
            .interfaceOffset = offsetof(Foo, interface),
            .interfaceSize = sizeof(FooInterface),
            .initialize = initialize,
        });
    });

    return clazz;
}

#undef _Class
```

### Key Macros (from Objectively)

| Macro | Meaning |
|---|---|
| `alloc(Foo)` | Allocate + zero-init; refcount = 1 |
| `retain(obj)` | Increment refcount |
| `release(obj)` | Decrement refcount; calls `dealloc` at 0 |
| `$(obj, method, ...)` | Virtual method dispatch via vtable |
| `super(Type, self, method, ...)` | Call superclass implementation |
| `cast(Type, obj)` | Runtime-checked downcast |
| `instanceof(Type, obj)` | Runtime type check |
| `ident` | `void *` — Objectively's generic pointer |

### Memory Rules

- `alloc` returns a retained object (refcount = 1); caller owns it.
- Every `retain` must be balanced by a `release`.
- `dealloc` must `release` every object field it owns, then call `super(Object, self, dealloc)` **last**.
- SDL GPU resources (`SDL_GPUBuffer`, `SDL_GPUTexture`, etc.) are **not** Objectively objects — release them through `RenderDevice` methods, not `release()`.

### Collections

| Type | Use for |
|---|---|
| `Array` / `MutableArray` | Objectively objects (`ident`) |
| `Vector` | Plain C structs / scalars (stores by value, not pointer) |
| `Set` / `MutableSet` | Object sets with hash/equality |
| `Dictionary` / `MutableDictionary` | Object key→value maps |

`Vector` is the correct choice for GPU vertex staging, draw call queues, and any array of plain C structs.

---

## Project Layout

```
Sources/ObjectivelyGPU/           Core classes
  Types.h                         GPU_Assert, SDL_Size, MakeRect/Size/Point
  RenderDevice.{h,c}              Owns SDL_GPUDevice, window, and frame loop
  CommandBuffer.{h,c}             Wraps SDL_GPUCommandBuffer for one frame
  RenderPass.{h,c}                Wraps SDL_GPURenderPass (draw commands)
  CopyPass.{h,c}                  Wraps SDL_GPUCopyPass (CPU→GPU uploads)
  ComputePass.{h,c}               Wraps SDL_GPUComputePass
Sources/ObjectivelyGPU/Shaders/   HLSL sources compiled offline with sdl-shadercross
```

---

## SDL3 GPU Conventions

### Pass Lifecycle

Passes are Objectively objects. Begin them via `CommandBuffer`, end them by calling `release()` — `dealloc` calls `SDL_EndGPU*Pass` automatically:

```c
CopyPass *copy = $(cmd, beginCopyPass);
// ... upload operations ...
release(copy);

RenderPass *pass = $(cmd, beginRenderPass, colorTargets, numTargets, NULL);
// ... draw calls ...
release(pass);
```

### Shader Loading

`RenderDevice::loadShader` appends the platform-appropriate extension automatically:
- macOS/iOS → `.msl`
- Linux/Android → `.spv`
- Windows → `.dxil`

The caller sets `stage`, `entrypoint`, and binding counts; `code`, `code_size`, and `format` are filled in by `loadShader`. Shader blobs are produced offline from HLSL source with `sdl-shadercross`.

### GPU Resource Lifetime

All `SDL_GPU*` resources (buffers, textures, pipelines, samplers, shaders, transfer buffers, fences) are released through the corresponding `RenderDevice::release*` method, never through `release()`.

---

## ObjectivelyMVC Integration

ObjectivelyGPU is the rendering backend for ObjectivelyMVC's `Renderer`. Understanding both sides of the seam is essential.

### Frame Loop (ObjectivelyMVC's perspective)

```
WindowController::render()
  ├─ Renderer::beginFrame()        ← acquires CommandBuffer + Swapchain from RenderDevice
  ├─ View::applyThemeIfNeeded()    ← CSS-like style cascade (recursive)
  ├─ View::layoutIfNeeded()        ← layout pass (recursive)
  ├─ View::draw(renderer)          ← tree traversal; each View calls Renderer draw primitives
  │   └─ Renderer::drawRectFilled(), drawTexture(), drawLines(), …
  │       └─ Accumulates MVC_Vertex data + MVC_DrawCall records into Vectors
  └─ Renderer::endFrame()
      ├─ CopyPass: uploads vertex buffer CPU→GPU
      ├─ RenderPass: executes each MVC_DrawCall
      │   (setScissor, bindTexture+sampler, pushUniform, drawPrimitives)
      └─ RenderDevice::submit(cmd)  ← SDL_SubmitGPUCommandBuffer → present
```

### ViewController GPU Lifecycle Hooks

When the `SDL_GPUDevice` is (re-)created or destroyed, ObjectivelyMVC propagates the event down the ViewController tree. Override these to manage GPU resources tied to a screen:

```c
// In a ViewController subclass interface:
void (*renderDeviceWillReset)(ViewController *self);  // free GPU resources
void (*renderDeviceDidReset)(ViewController *self);   // allocate GPU resources
```

Both hooks call `super` and propagate to child VCs automatically.

### View::render() Override Point

Views that need custom GPU output override `render(renderer)`. All draw calls funnel through `Renderer` primitives which accumulate into the deferred draw call queue — Views never touch `SDL_GPU*` APIs directly.

---

## Doxygen

All public API must have Doxygen comments:

- `@file` + `@brief` at the top of every header.
- Every interface function pointer gets a full `@fn ClassName::methodName(...)` block with `@param`, `@return`, `@details`, and `@memberof`.
- Struct documentation uses `@extends ClassName` on the class brief.
- Field tags: `@private` (internal, no external access), `@protected` (subclass-accessible).
- Pragma marks group sections in `.c` files: `#pragma mark - Object`, `#pragma mark - ClassName`, `#pragma mark - Class lifecycle`.
