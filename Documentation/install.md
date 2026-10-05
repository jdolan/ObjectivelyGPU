Installing ObjectivelyGPU {#install}
=========================

Dependencies, building, and linking against ObjectivelyGPU.

[TOC]

## Releases

Tagged releases are published on the [GitHub releases page](https://github.com/jdolan/ObjectivelyGPU/releases). To build the latest from source, follow the steps below.

## Dependencies

* [Objectively](https://github.com/jdolan/Objectively) >= 2.2.0
* [SDL3](https://github.com/libsdl-org/SDL) >= 3.4.0, from the `ObjectivelyGPU` tag of [jdolan/SDL](https://github.com/jdolan/SDL) for occlusion queries. See "SDL3: the jdolan/SDL fork" below.

## SDL3: the jdolan/SDL fork

### Why a fork

`QueryPool` and occlusion queries need the SDL_gpu query API (`SDL_GPU_QUERY_API`). Upstream SDL does not
ship it yet: the upstream pull requests ([libsdl-org/SDL#15651](https://github.com/libsdl-org/SDL/pull/15651),
[thatcosmonaut/SDL#247](https://github.com/thatcosmonaut/SDL/pull/247)) are stalled. The `ObjectivelyGPU` tag in
[jdolan/SDL](https://github.com/jdolan/SDL) carries an SDL release plus that API, Metal and Vulkan
implementations of it, and D3D12 stubs. The tag currently points at branch `objectivelygpu-3.4.18`: SDL 3.4.18
and the query commits.

ObjectivelyGPU and the applications that use its occlusion queries, such as [Quetoo](https://github.com/jdolan/quetoo),
need the fork. ObjectivelyMVC does not use queries, and builds against any SDL3.

Against any other SDL3, ObjectivelyGPU still builds and runs, but silently without queries:

* `QueryPool.c` emits a single compiler warning, `SDL3 lacks SDL_GPU_QUERY_API`.
* `RenderPass::beginQuery` and `endQuery` do nothing.
* `CopyPass::downloadQueryResults` writes a "not occluded" result for every query.

On D3D12 the fork's query functions are stubs that fail, so `RenderDevice::createQueryPool` fails its
`GPU_Assert` and exits. Use Vulkan for queries on Windows.

### Linux and macOS (autotools)

Build the fork from source, and install it to `/usr/local`:

```sh
git clone --branch ObjectivelyGPU https://github.com/jdolan/SDL.git ../SDL
cmake -S ../SDL -B ../SDL/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
cmake --build ../SDL/build -j8
sudo cmake --install ../SDL/build
```

On Linux, run `sudo ldconfig` after the install.

#### The fork MUST win the `pkg-config` search

On macOS, Homebrew's `pkg-config` searches `/opt/homebrew/lib/pkgconfig` **before** `/usr/local/lib/pkgconfig`.
Other Homebrew formulae (`ffmpeg`, `sdl2-compat`, `sdl3_image`) depend on Homebrew's `sdl3`, so it is often
installed, and its stock `sdl3.pc` then wins. The build succeeds, links stock SDL, and has no queries. Put
`/usr/local` first in your shell profile (`~/.zprofile` for zsh):

```sh
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
```

`pkg-config` also supplies the runtime path. The fork's `libSDL3.0.dylib` has the install name
`@rpath/libSDL3.0.dylib`, and `sdl3.pc` adds `-Wl,-rpath,/usr/local/lib`. A program or library that links SDL3
without `pkg-config` has no rpath, and fails at launch with `Library not loaded: @rpath/libSDL3.0.dylib`.

Do not install the fork into the Homebrew keg (`Cellar/sdl3`). `brew upgrade` or `brew reinstall` replaces it
with stock SDL without a warning.

#### SDL3_image and SDL3_ttf MUST be built against the fork

Homebrew's `sdl3_image` and `sdl3_ttf` link Homebrew's `libSDL3`. A program that uses them together with the
fork loads **two** copies of SDL3, with separate state. Build both from source against the fork, and install
them to `/usr/local`. Use the release tags that CI uses (see ObjectivelyMVC's `.github/workflows/build.yml`):

```sh
for lib in SDL_image:release-3.4.4 SDL_ttf:release-3.2.2; do
  git clone --depth 1 --branch "${lib#*:}" "https://github.com/libsdl-org/${lib%%:*}.git" "../${lib%%:*}"
  cmake -S "../${lib%%:*}" -B "../${lib%%:*}/build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local -DSDL3_DIR=/usr/local/lib/cmake/SDL3 \
    -DCMAKE_INSTALL_NAME_DIR=/usr/local/lib
  cmake --build "../${lib%%:*}/build" -j8
  sudo cmake --install "../${lib%%:*}/build"
done
```

#### Verify the installation

```sh
pkg-config --modversion --variable=libdir sdl3            # 3.4.18 and /usr/local/lib
nm -gU /usr/local/lib/libSDL3.0.dylib | grep SDL_CreateGPUQueryPool
otool -L /usr/local/lib/libObjectivelyGPU.dylib | grep SDL3  # after `make install`
DYLD_PRINT_LIBRARIES=1 ./your-program 2>&1 | grep SDL3     # exactly one libSDL3, from /usr/local/lib
```

### macOS and iOS (Xcode)

`ObjectivelyGPU.xcworkspace` builds `SDL3.framework` from SDL's own `Xcode/SDL/SDL.xcodeproj`, in the sibling
checkout `../SDL`. The workspaces of ObjectivelyMVC and Quetoo also put `$(HOMEBREW_PREFIX)/include` on the
header search path, and there `$(SRCROOT)/../SDL/include` MUST stay ahead of it. If Homebrew's stock SDL headers win, the fork's `SDL_GPUDepthStencilTargetInfo`
(which adds `query_pool`) has a different size in each compilation unit, and ObjectivelyGPU writes past its
callers' structs.

### Windows (Visual Studio)

`ObjectivelyGPU.vs15/sdl3.targets` downloads `SDL3-devel-VC.zip` from the tag's release on first build, into
`ObjectivelyGPU.vs15/libs/`. That cache is never refreshed. Delete `libs/` to pick up a moved tag.

### CI

The macOS and Linux jobs in `.github/workflows/build.yml` clone the tag and build it with CMake. macOS CI
installs it into the Homebrew prefix. Locally, use `/usr/local`, as above. The Windows job uses `sdl3.targets`. The release workflow checks out the tag to `SDL` and builds the
xcframework from it.

### Moving the tag

To change the SDL3 revision for the whole stack, move the tag, then publish the Windows artifacts. The publish
run MUST finish before any Windows build, because until then the release still serves the previous assets.

```sh
git tag -f ObjectivelyGPU <rev> && git push -f origin ObjectivelyGPU
gh workflow run objectivelygpu.yml -R jdolan/SDL
gh run watch -R jdolan/SDL --exit-status "$(gh run list -R jdolan/SDL -w objectivelygpu.yml -L 1 --json databaseId -q '.[0].databaseId')"
```

Then update each local checkout, and rebuild and reinstall SDL3, SDL3_image, SDL3_ttf and everything above them:

```sh
git -C ../SDL fetch --force --tags && git -C ../SDL checkout ObjectivelyGPU
```

## Building

```sh
autoreconf -i
./configure
make && sudo make install
```

## Shaders (SDL_shadercross)

ObjectivelyGPU consumes compiled shader blobs, not GLSL source: SPIR-V for Vulkan, MSL for Metal, and DXIL for D3D12. The toolchain is:

1. Author your shaders in GLSL.
2. Compile GLSL to SPIR-V with `glslc` (from [shaderc](https://github.com/google/shaderc)).
3. Cross-compile SPIR-V to MSL or DXIL with `shadercross` (from [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross)).

`glslc` ships with Homebrew's `shaderc`. `shadercross` must be built from source.

### Building shadercross

Building `shadercross` requires the SDL3 development headers and libraries — the same SDL3 that ObjectivelyGPU depends on.

```sh
git clone https://github.com/libsdl-org/SDL_shadercross
cd SDL_shadercross
# Vendored deps. The DirectXShaderCompiler submodule vendors LLVM/Clang and is a
# multi-gigabyte, lengthy build — it is only needed for HLSL input and DXIL output.
git submodule update --init --recursive

cmake -S . -B build \
  -DSDLSHADERCROSS_VENDORED=ON \
  -DSDLSHADERCROSS_SPIRVCROSS_SHARED=OFF \
  -DSDLSHADERCROSS_CLI=ON \
  -DSDLSHADERCROSS_DXC=ON \
  -DSDLSHADERCROSS_INSTALL=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_RPATH='@loader_path/../lib;/usr/local/lib' \
  -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=ON

cmake --build build -j$(sysctl -n hw.ncpu)
sudo cmake --install build
```

The two `CMAKE_INSTALL_RPATH` options are essential. Without them the installed `shadercross` has no `LC_RPATH` and fails at runtime with `Library not loaded: @rpath/libSDL3_shadercross.0.dylib`. Setting the install rpath to `@loader_path/../lib` (relocatable) lets the installed binary in `bin/` find `libSDL3_shadercross` in the sibling `lib/`. On Linux, use `$ORIGIN/../lib` in place of `@loader_path/../lib`.

If you do not need HLSL input or DXIL output (for example, Metal and Vulkan only), pass `-DSDLSHADERCROSS_DXC=OFF` and skip the DirectXShaderCompiler submodule. This avoids the enormous LLVM build entirely and is the recommended lighter-weight path when D3D12 support is not required.

The installed command-line tool is named `shadercross` (not `sdl-shadercross`).

### Transpiling shaders

Compile GLSL to SPIR-V, then cross-compile SPIR-V to the target language:

```sh
glslc -fshader-stage=vertex my.vert.glsl -o my.vert.spv
shadercross my.vert.spv -s SPIRV -d MSL -t vertex --msl-version 2.1.0 -o my.vert.metal
```

Pass `--msl-version 2.1.0` for shaders that use features such as `invariant gl_Position`; older MSL versions reject them.

## Linking

Compile and link against ObjectivelyGPU with `pkg-config`:

```sh
gcc `pkg-config --cflags --libs ObjectivelyGPU` -o myprogram *.c
```
