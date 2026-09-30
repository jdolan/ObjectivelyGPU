Installing ObjectivelyGPU {#install}
=========================

Dependencies, building, and linking against ObjectivelyGPU.

[TOC]

## Releases

Tagged releases are published on the [GitHub releases page](https://github.com/jdolan/ObjectivelyGPU/releases). To build the latest from source, follow the steps below.

## Dependencies

* [Objectively](https://github.com/jdolan/Objectively) >= 2.0.0
* [SDL3](https://github.com/libsdl-org/SDL) >= 3.2.0

### SDL3 and occlusion queries

`QueryPool` requires the SDL_gpu query API (`SDL_GPU_QUERY_API`), which upstream SDL does not ship yet. The
`ObjectivelyGPU` tag in [jdolan/SDL](https://github.com/jdolan/SDL) carries it, and CI for ObjectivelyGPU,
ObjectivelyMVC and Quetoo MUST build against that tag. Against any other SDL3, ObjectivelyGPU still builds, but
`QueryPool.c` emits a compiler warning and occlusion queries are disabled.

* Linux and macOS (autotools): build SDL from source with
  `git clone --branch ObjectivelyGPU https://github.com/jdolan/SDL.git`.
* macOS and iOS (Xcode): `Frameworks/fetch-sdl3.sh` downloads `SDL3.xcframework` from the tag's release. It
  records the tag's commit, and downloads again if the tag moves.
* Windows (Visual Studio): `ObjectivelyGPU.vs15/sdl3.targets` downloads `SDL3-devel-VC.zip` from the tag's
  release on first build. Delete `ObjectivelyGPU.vs15/libs/` to pick up a moved tag.

To change the SDL3 revision for the whole stack, move the tag, then publish its artifacts. The publish run
MUST finish before any consumer builds, because until then the release still serves the previous assets.
`fetch-sdl3.sh` refuses a release that was built from another commit.

```sh
git tag -f ObjectivelyGPU <rev> && git push -f origin ObjectivelyGPU
gh workflow run objectivelygpu.yml -R jdolan/SDL
gh run watch -R jdolan/SDL --exit-status "$(gh run list -R jdolan/SDL -w objectivelygpu.yml -L 1 --json databaseId -q '.[0].databaseId')"
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
