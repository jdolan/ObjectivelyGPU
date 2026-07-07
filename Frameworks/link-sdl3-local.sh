#!/usr/bin/env bash
#
# link-sdl3-local.sh — assembles Frameworks/SDL3.xcframework.local, an
# xcframework whose macOS slice is a live symlink into a locally-built SDL3
# checkout, instead of the officially distributed binary cached by
# fetch-sdl3.sh.
#
# Defaults to Xcode/SDL/build/Release/SDL3.framework, produced by building the
# "SDL3" target/scheme in Xcode/SDL/SDL.xcodeproj with the Release
# configuration -- this is the same project, target, and configuration
# libsdl.org's own release process uses (see build-scripts/build-release.py's
# create_dmg() and build-scripts/release-info.json's "dmg" section), so it's
# the closest thing to a "vanilla local rebuild" of an official release,
# including your working-tree changes. If you build via CMake instead, point
# SDL3_LOCAL_BUILD at that framework instead.
#
# Because the macOS slice is a symlink (not a copy), rebuilding SDL3 in the
# local checkout is picked up immediately by Xcode -- there's no need to
# re-run this script after every SDL rebuild. Re-run it only if
# SDL3_LOCAL_BUILD's *path* changes, or after deleting
# Frameworks/SDL3.xcframework.local.
#
# Use use-sdl3.sh to toggle Frameworks/SDL3.xcframework between this and the
# official release fetched by fetch-sdl3.sh.
#
set -euo pipefail

SDL3_LOCAL_BUILD="${SDL3_LOCAL_BUILD:-$HOME/Coding/SDL/Xcode/SDL/build/Release/SDL3.framework}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STABLE="$SCRIPT_DIR/SDL3.xcframework.stable"
LOCAL="$SCRIPT_DIR/SDL3.xcframework.local"

if [ ! -d "$SDL3_LOCAL_BUILD" ]; then
    echo "error: $SDL3_LOCAL_BUILD not found (set SDL3_LOCAL_BUILD to override)" >&2
    exit 1
fi

# The official xcframework ships headers only at Headers/SDL3/*.h (so that a
# flat -I onto that Headers/ dir resolves the project's #include <SDL3/*.h>
# convention). A vanilla CMake-built SDL3.framework has no such nesting.
# Fix that up once, in place, with a self-referential symlink. "." is used
# (rather than an absolute/canonicalized path) so it resolves correctly
# regardless of the Headers/Versions/Current symlink chain used to reach it;
# macOS's BSD readlink has no reliable -f (canonicalize) flag to do that
# resolution ourselves.
HEADERS="$SDL3_LOCAL_BUILD/Headers"
if [ ! -e "$HEADERS/SDL_gpu.h" ]; then
    echo "error: $HEADERS doesn't look like a built SDL3 framework (no SDL_gpu.h)." >&2
    echo "       Is $SDL3_LOCAL_BUILD mid-rebuild? Finish 'cmake --build' there and re-run." >&2
    exit 1
fi
if [ ! -e "$HEADERS/SDL3" ]; then
    echo "==> Patching $HEADERS with a self-referential SDL3/ symlink"
    ln -s . "$HEADERS/SDL3"
fi

# The .stable cache is our template for Info.plist and the iOS slices, which
# the local checkout doesn't build. Make sure it exists.
[ -d "$STABLE" ] || "$SCRIPT_DIR/fetch-sdl3.sh"

echo "==> Assembling Frameworks/SDL3.xcframework.local"
rm -rf "$LOCAL"
mkdir "$LOCAL"
cp "$STABLE/Info.plist" "$LOCAL/Info.plist"

for slice in "$STABLE"/*/; do
    slice="${slice%/}"
    name="$(basename "$slice")"
    case "$name" in
        macos-*)
            mkdir "$LOCAL/$name"
            ln -s "$SDL3_LOCAL_BUILD" "$LOCAL/$name/SDL3.framework"
            ;;
        *)
            # No local build for iOS/simulator slices; fall back to stable.
            ln -s "../SDL3.xcframework.stable/$name" "$LOCAL/$name"
            ;;
    esac
done

echo "==> Frameworks/SDL3.xcframework.local ready (macOS slice -> $SDL3_LOCAL_BUILD)"
