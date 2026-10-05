# ObjectivelyGPU

ObjectivelyGPU is an object-oriented wrapper over SDL3's GPU API (SDL_gpu), written in GNU C on
[Objectively](https://github.com/jdolan/Objectively). It provides the render device, command buffers,
passes, pipelines, buffers, textures, framebuffers and occlusion queries that
[ObjectivelyMVC](https://github.com/jdolan/ObjectivelyMVC) and [Quetoo](https://github.com/jdolan/quetoo)
render with.

This file is the shared instruction set for coding agents. Read this first.

It deliberately records only what a careful reading of the code does **not** reveal: rules that fail
silently, constraints that live outside this repository, and ordering that no call site shows. For
the object model (`$`, `super`, `alloc`, `release`, the anatomy of a class), read Objectively's
`AGENTS.md` and `Documentation/guide.md`. For anything else, read the code.

## SDL3 is a fork

Occlusion queries need the SDL_gpu query API, which upstream SDL does not ship yet. The stack builds
against the `ObjectivelyGPU` tag of [jdolan/SDL](https://github.com/jdolan/SDL), checked out beside
this repository as `../SDL`. [`Documentation/install.md`](Documentation/install.md) is the canonical
procedure for the whole stack: why the fork exists, how to install it, how CI gets it, and how to
move the tag. Read it before you change anything about SDL.

The failures are silent:

- **Against stock SDL, queries silently do nothing.** The build emits one warning,
  `SDL3 lacks SDL_GPU_QUERY_API`. `beginQuery` and `endQuery` are no-ops, and
  `downloadQueryResults` reports every query as "not occluded". Nothing else changes.
- **On macOS, Homebrew's stock `sdl3.pc` wins the `pkg-config` search** unless `PKG_CONFIG_PATH`
  puts `/usr/local/lib/pkgconfig` first. The result builds, links stock SDL, and has no queries.
- **The fork's headers MUST come first.** The fork adds `query_pool` to
  `SDL_GPUDepthStencilTargetInfo`. If one compilation unit sees Homebrew's stock headers, the struct
  has two sizes, and ObjectivelyGPU writes past its callers' structs. The Xcode projects of
  ObjectivelyMVC and Quetoo put `$(HOMEBREW_PREFIX)/include` on the header search path, and there
  `$(SRCROOT)/../SDL/include` MUST precede it.
- **D3D12 has no queries.** The fork's D3D12 functions are stubs that fail, so
  `RenderDevice::createQueryPool` fails its `GPU_Assert` and exits.
- `configure.ac` checks only for `sdl3 >= 3.2.0`. It does not detect the fork.

## Rules that fail silently

- **Every GPU resource is an Objectively object.** `Buffer`, `Texture`, `Sampler`, `Shader`, the
  pipelines, `TransferBuffer`, `Fence` and `QueryPool` release their SDL handle in `dealloc`. Free
  them with `release()`. Do not call `SDL_ReleaseGPU*` on a handle that one of them owns.
- **A resource holds a weak reference to its `RenderDevice`.** It MUST NOT outlive the device, or its
  `dealloc` uses a destroyed device. A resource MUST NOT retain the device either: that makes a
  cycle, and shutting down the device leaks every resource in it.
- **A pass ends when it is released.** `RenderPass`, `CopyPass` and `ComputePass` call
  `SDL_EndGPU*Pass` in `dealloc`. A pass that is still retained stays open.
- **A query MUST begin and end in the render pass that names its pool.** Set
  `depthStencil.query_pool` when you begin the pass. `beginRenderPassWithFramebuffer` and
  `Framebuffer::depthTargetInfo` do not set it, so call `beginRenderPass` with your own target
  infos. The pass resets every query in the pool when it begins, so a query that you download MUST
  be begun in that same pass.
- **Acquire the swapchain only to present.** `RenderDevice::endFrame` acquires it, blits and
  submits. A swapchain texture held across the frame starves `CAMetalLayer`, and frames are silently
  dropped.
- **`CopyPass::uploadData` creates a transfer buffer for each call.** Per-frame uploads SHOULD own a
  `TransferBuffer` and reuse it.
- **`RenderDevice::loadShader` picks the file extension.** It tries `.metallib`, `.metal`, `.dxil`
  and `.spv`, in that order, filtered by the formats the device accepts. If you pass a NULL
  `entrypoint`, it uses `main0` for Metal and `main` for SPIR-V and DXIL.
- **Windows symbol exports.** Exported `const` data needs `OBJECTIVELYGPU_EXPORT_DATA`. A static
  function MUST NOT be named `write` or another CRT function name, because it clashes with the UCRT.

## Shaders

Shaders are authored in GLSL, compiled to SPIR-V with `glslc`, and cross-compiled to MSL (and DXIL)
with `shadercross`. The compiled blobs are committed beside their sources. After you edit a `.glsl`
file, run `make -C Examples shaders` and commit the outputs. `Documentation/install.md` explains how
to build `shadercross`.

## Building

```sh
autoreconf -i
./configure
make -j$(sysctl -n hw.logicalcpu)
sudo make install
```

There is no test suite. `make` builds `Examples/Hello` and `Examples/HelloCompute`, which are the
only programs that exercise the library here.

Downstream projects consume the **installed** library from `/usr/local` through `pkg-config`
(package name `ObjectivelyGPU`). A change here reaches them only after `sudo make install`. The
soname is `-release MAJOR.MINOR` from `AC_INIT`, so a layout change MUST bump the minor version, and
downstream projects MUST be rebuilt.

### A new source file goes in four places

1. `Sources/ObjectivelyGPU/Makefile.am`, in both `pkginclude_HEADERS` and
   `libObjectivelyGPU_la_SOURCES`.
2. `ObjectivelyGPU.xcodeproj/project.pbxproj`.
3. `ObjectivelyGPU.vs15/ObjectivelyGPU.vcxproj` (`ClInclude` and `ClCompile`). A file missing here
   fails the Windows link with an undefined symbol.
4. The umbrella header `Sources/ObjectivelyGPU.h`.

### CI

- `build.yml` builds macOS, Linux and Windows on pushes and pull requests to `main`, and on
  `v*.*.*` tags. It only builds; it runs nothing.
- `release.yml` runs on `v*.*.*` tags and builds the macOS, iOS and simulator xcframework.
- `docs.yml` publishes the Doxygen documentation to GitHub Pages.
