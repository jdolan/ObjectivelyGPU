#!/usr/bin/env bash
#
# fetch-sdl3.sh — downloads SDL3.xcframework from the jdolan/SDL release for SDL3_TAG and caches it
# under Frameworks/.
#
# The whole stack (ObjectivelyGPU, ObjectivelyMVC, Quetoo) pins SDL3 to one tag in jdolan/SDL, which
# carries the SDL_gpu query API. The tag's commit SHA is stored next to the cache, so moving the tag
# causes a new download. This mirrors ObjectivelyGPU.vs15/sdl3.targets on Windows.
#
set -euo pipefail

SDL3_REPO="${SDL3_REPO:-jdolan/SDL}"
SDL3_TAG="${SDL3_TAG:-ObjectivelyGPU}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
XCFRAMEWORK="$SCRIPT_DIR/SDL3.xcframework"
STAMP="$SCRIPT_DIR/SDL3.xcframework.sha"

sha="$(git ls-remote "https://github.com/$SDL3_REPO.git" "refs/tags/$SDL3_TAG" | cut -f1)"
if [ -z "$sha" ]; then
    echo "error: tag $SDL3_TAG not found in $SDL3_REPO" >&2
    exit 1
fi

cached() {
    [ -d "$XCFRAMEWORK" ] && [ ! -L "$XCFRAMEWORK" ] && [ "$(cat "$STAMP" 2>/dev/null)" = "$sha" ]
}

if cached; then
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
if cached; then
    exit 0
fi

DMG_URL="https://github.com/$SDL3_REPO/releases/download/$SDL3_TAG/SDL3.dmg"

echo "==> Downloading SDL3 $SDL3_REPO@$SDL3_TAG ($sha)"
curl -fL --retry 3 "$DMG_URL" -o "$TMP/SDL3.dmg"

echo "==> Mounting SDL3.dmg"
mkdir -p "$MNT"
hdiutil attach "$TMP/SDL3.dmg" -nobrowse -quiet -mountpoint "$MNT"

src="$(find "$MNT" -maxdepth 2 -name SDL3.xcframework -type d | head -1)"
if [ -z "$src" ]; then
    echo "error: SDL3.xcframework not found in $DMG_URL" >&2
    exit 1
fi

# Replace the cache, and the .stable/.local directories of the former use-sdl3.sh layout.
echo "==> Caching SDL3.xcframework"
rm -rf "$XCFRAMEWORK" "$SCRIPT_DIR/SDL3.xcframework.stable" "$SCRIPT_DIR/SDL3.xcframework.local"
cp -R "$src" "$XCFRAMEWORK"
echo "$sha" > "$STAMP"

echo "==> SDL3.xcframework $SDL3_TAG ready at Frameworks/SDL3.xcframework"
