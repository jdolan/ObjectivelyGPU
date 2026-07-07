#!/usr/bin/env bash
#
# use-sdl3.sh — toggles Frameworks/SDL3.xcframework between the official
# stable release (fetch-sdl3.sh) and a locally-built SDL3 checkout
# (link-sdl3-local.sh).
#
# ObjectivelyGPU/Frameworks/SDL3.xcframework is the single copy of SDL3 that
# ObjectivelyGPU, ObjectivelyMVC, and Quetoo all link against (ObjectivelyMVC's
# own Frameworks/SDL3.xcframework is a symlink back to this one; Quetoo links
# this path directly). Running this script here is enough to flip SDL3 for
# all three projects at once -- nothing to run in the sibling repos.
#
# Usage:
#   ./use-sdl3.sh stable   # official release from libsdl.org (fetch-sdl3.sh)
#   ./use-sdl3.sh local    # your local SDL checkout (link-sdl3-local.sh)
#   ./use-sdl3.sh          # print which one is currently active
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
XCFRAMEWORK="$SCRIPT_DIR/SDL3.xcframework"

status() {
    if [ -L "$XCFRAMEWORK" ]; then
        echo "SDL3.xcframework -> $(readlink "$XCFRAMEWORK")"
    elif [ -d "$XCFRAMEWORK" ]; then
        echo "SDL3.xcframework is a plain directory (not managed by use-sdl3.sh yet; run fetch-sdl3.sh)"
    else
        echo "SDL3.xcframework is not set up yet; run '$0 stable' or '$0 local'"
    fi
}

case "${1:-}" in
    stable)
        "$SCRIPT_DIR/fetch-sdl3.sh"
        status
        ;;
    local)
        "$SCRIPT_DIR/link-sdl3-local.sh"
        ln -sfn SDL3.xcframework.local "$XCFRAMEWORK"
        status
        ;;
    "")
        status
        ;;
    *)
        echo "usage: $0 [stable|local]" >&2
        exit 1
        ;;
esac
