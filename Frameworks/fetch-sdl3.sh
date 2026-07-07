#!/usr/bin/env bash
#
# fetch-sdl3.sh — downloads the official SDL3 Apple xcframework from libsdl.org
# on demand and caches it under Frameworks/.
#
# This mirrors the Windows VS build (ObjectivelyGPU.vs15/sdl3.targets): CI and
# local developers share a single code path. The download is cached at
# Frameworks/SDL3.xcframework.stable, and Frameworks/SDL3.xcframework is a
# symlink to it. This indirection lets link-sdl3-local.sh (and use-sdl3.sh)
# repoint that same symlink at a locally-built SDL3 checkout without disturbing
# this cache. Bump SDL3_VERSION to upgrade; delete
# Frameworks/SDL3.xcframework.stable to force a re-download.
#
set -euo pipefail

SDL3_VERSION="${SDL3_VERSION:-3.4.2}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
STABLE="$SCRIPT_DIR/SDL3.xcframework.stable"
XCFRAMEWORK="$SCRIPT_DIR/SDL3.xcframework"

# Migrate a pre-existing plain directory (from before the .stable split) in place.
if [ -d "$XCFRAMEWORK" ] && [ ! -L "$XCFRAMEWORK" ]; then
    mv "$XCFRAMEWORK" "$STABLE"
fi

# Point the symlink at the stable cache, creating/replacing it as needed.
relink() {
    ln -sfn "$(basename "$STABLE")" "$XCFRAMEWORK"
}

# Already cached? Just make sure the symlink points at it and we're done.
if [ -d "$STABLE" ]; then
    relink
    exit 0
fi

# Serialize concurrent invocations (parallel target builds) on a lock dir,
# the shell equivalent of the Global mutex used by sdl3.targets on Windows.
LOCK="$SCRIPT_DIR/.sdl3.lock"
while ! mkdir "$LOCK" 2>/dev/null; do
    sleep 1
done

TMP="$(mktemp -d)"
MNT="$TMP/mnt"

cleanup() {
    hdiutil detach "$MNT" -quiet 2>/dev/null || true
    rm -rf "$TMP"
    rmdir "$LOCK" 2>/dev/null || true
}
trap cleanup EXIT

# Re-check after acquiring the lock; another build may have just finished.
if [ -d "$STABLE" ]; then
    relink
    exit 0
fi

DMG_URL="https://github.com/libsdl-org/SDL/releases/download/release-$SDL3_VERSION/SDL3-$SDL3_VERSION.dmg"

echo "==> Downloading SDL3 $SDL3_VERSION"
curl -fL --retry 3 "$DMG_URL" -o "$TMP/SDL3.dmg"

echo "==> Mounting SDL3 $SDL3_VERSION"
mkdir -p "$MNT"
hdiutil attach "$TMP/SDL3.dmg" -nobrowse -quiet -mountpoint "$MNT"

src="$(find "$MNT" -maxdepth 2 -name SDL3.xcframework -type d | head -1)"
if [ -z "$src" ]; then
    echo "error: SDL3.xcframework not found in $DMG_URL" >&2
    exit 1
fi

echo "==> Caching SDL3.xcframework"
cp -R "$src" "$STABLE"
relink

echo "==> SDL3.xcframework $SDL3_VERSION ready at Frameworks/SDL3.xcframework"
