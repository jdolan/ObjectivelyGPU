#!/usr/bin/env bash
#
# build-xcframework.sh — builds ObjectivelyGPU.xcframework for iOS device,
# iOS Simulator, and macOS, then assembles an xcframework.
#
# Assumes Frameworks/SDL3.xcframework and Frameworks/Objectively.xcframework
# are already present (they ship in the repo).
#
# Usage:
#   scripts/build-xcframework.sh
#
set -e -o pipefail

export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GPU_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$GPU_DIR/.build-xcframework"
FRAMEWORKS_DIR="$GPU_DIR/Frameworks"
FRAMEWORK="ObjectivelyGPU"
OUTPUT="$FRAMEWORKS_DIR/$FRAMEWORK.xcframework"

IOS_MIN="${IOS_MIN:-14.0}"
MACOS_MIN="${MACOS_MIN:-12.0}"

CLANG=$(xcrun --find clang)
LIPO=$(xcrun --find lipo)
NPROC=$(sysctl -n hw.logicalcpu)
MACOSX_SDK=$(xcrun --sdk macosx --show-sdk-path)
IPHONEOS_SDK=$(xcrun --sdk iphoneos --show-sdk-path)
IPHONESIMULATOR_SDK=$(xcrun --sdk iphonesimulator --show-sdk-path)

SOURCES=(
    "$GPU_DIR/Sources/ObjectivelyGPU/CommandBuffer.c"
    "$GPU_DIR/Sources/ObjectivelyGPU/ComputePass.c"
    "$GPU_DIR/Sources/ObjectivelyGPU/CopyPass.c"
    "$GPU_DIR/Sources/ObjectivelyGPU/RenderDevice.c"
    "$GPU_DIR/Sources/ObjectivelyGPU/RenderPass.c"
)

mkdir -p "$BUILD_DIR" "$FRAMEWORKS_DIR"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

fw_headers() {
    local xcfw="$1" slice="$2"
    echo "$FRAMEWORKS_DIR/$xcfw.xcframework/$slice/$xcfw.framework/Headers"
}

make_info_plist() {
    local fwdir="$1" min_key="$2" min_ver="$3"
    cat > "$fwdir/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key><string>en</string>
    <key>CFBundleExecutable</key><string>$FRAMEWORK</string>
    <key>CFBundleIdentifier</key><string>com.github.jdolan.$FRAMEWORK</string>
    <key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
    <key>CFBundleName</key><string>$FRAMEWORK</string>
    <key>CFBundlePackageType</key><string>FMWK</string>
    <key>CFBundleShortVersionString</key><string>1.0</string>
    <key>CFBundleVersion</key><string>1</string>
    <key>$min_key</key><string>$min_ver</string>
</dict>
</plist>
EOF
}

build_slice() {
    local name="$1" sdk="$2" arch="$3" min_flag="$4" min_key="$5" min_ver="$6"
    local obj_slice="$7" sdl3_slice="$8"

    local objdir="$BUILD_DIR/objs-$name"
    local outdir="$BUILD_DIR/install-$name"
    local fwdir="$outdir/$FRAMEWORK.framework"

    if [ -f "$fwdir/$FRAMEWORK" ]; then
        echo "==> $FRAMEWORK ($name): cached"
        return 0
    fi

    echo "==> $FRAMEWORK ($name): compiling"
    mkdir -p "$objdir" "$fwdir/Headers"

    local obj_headers obj_fw_dir sdl3_headers sdl3_fw_dir
    obj_headers=$(fw_headers Objectively "$obj_slice")
    obj_fw_dir="$FRAMEWORKS_DIR/Objectively.xcframework/$obj_slice"
    sdl3_headers="$FRAMEWORKS_DIR/SDL3.xcframework/$sdl3_slice/SDL3.framework/Headers"
    sdl3_fw_dir="$FRAMEWORKS_DIR/SDL3.xcframework/$sdl3_slice"

    for src in "${SOURCES[@]}"; do
        "$CLANG" -arch "$arch" -isysroot "$sdk" $min_flag \
            -I"$GPU_DIR/Sources" \
            -I"$obj_headers" \
            -I"$sdl3_headers" \
            -c "$src" -o "$objdir/$(basename "$src" .c).o"
    done

    echo "==> $FRAMEWORK ($name): linking"
    "$CLANG" -arch "$arch" -isysroot "$sdk" $min_flag \
        -dynamiclib \
        -Wl,-install_name,@rpath/$FRAMEWORK.framework/$FRAMEWORK \
        -F"$sdl3_fw_dir" -framework SDL3 \
        -F"$obj_fw_dir"  -framework Objectively \
        "$objdir"/*.o \
        -o "$fwdir/$FRAMEWORK"

    cp "$GPU_DIR/Sources/ObjectivelyGPU/"*.h "$fwdir/Headers/"
    cp "$GPU_DIR/Sources/ObjectivelyGPU.h"   "$fwdir/Headers/"
    make_info_plist "$fwdir" "$min_key" "$min_ver"
}

# ---------------------------------------------------------------------------
# Build slices
# ---------------------------------------------------------------------------

build_slice "iphoneos-arm64" \
    "$IPHONEOS_SDK" "arm64" "-miphoneos-version-min=$IOS_MIN" \
    "MinimumOSVersion" "$IOS_MIN" \
    "ios-arm64" "ios-arm64"

build_slice "iphonesimulator-arm64" \
    "$IPHONESIMULATOR_SDK" "arm64" "-mios-simulator-version-min=$IOS_MIN" \
    "MinimumOSVersion" "$IOS_MIN" \
    "ios-arm64_x86_64-simulator" "ios-arm64_x86_64-simulator"

build_slice "iphonesimulator-x86_64" \
    "$IPHONESIMULATOR_SDK" "x86_64" "-mios-simulator-version-min=$IOS_MIN" \
    "MinimumOSVersion" "$IOS_MIN" \
    "ios-arm64_x86_64-simulator" "ios-arm64_x86_64-simulator"

build_slice "macos-arm64" \
    "$MACOSX_SDK" "arm64" "-mmacosx-version-min=$MACOS_MIN" \
    "LSMinimumSystemVersion" "$MACOS_MIN" \
    "macos-arm64_x86_64" "macos-arm64_x86_64"

build_slice "macos-x86_64" \
    "$MACOSX_SDK" "x86_64" "-mmacosx-version-min=$MACOS_MIN" \
    "LSMinimumSystemVersion" "$MACOS_MIN" \
    "macos-arm64_x86_64" "macos-arm64_x86_64"

# ---------------------------------------------------------------------------
# Lipo fat slices
# ---------------------------------------------------------------------------

echo "==> $FRAMEWORK: lipo iOS Simulator (arm64 + x86_64)"
SIM_DIR="$BUILD_DIR/install-iphonesimulator-fat"
mkdir -p "$SIM_DIR/$FRAMEWORK.framework/Headers"
"$LIPO" -create \
    "$BUILD_DIR/install-iphonesimulator-arm64/$FRAMEWORK.framework/$FRAMEWORK" \
    "$BUILD_DIR/install-iphonesimulator-x86_64/$FRAMEWORK.framework/$FRAMEWORK" \
    -output "$SIM_DIR/$FRAMEWORK.framework/$FRAMEWORK"
cp -r "$BUILD_DIR/install-iphoneos-arm64/$FRAMEWORK.framework/Headers/." \
    "$SIM_DIR/$FRAMEWORK.framework/Headers/"
cp "$BUILD_DIR/install-iphoneos-arm64/$FRAMEWORK.framework/Info.plist" \
    "$SIM_DIR/$FRAMEWORK.framework/"

echo "==> $FRAMEWORK: lipo macOS (arm64 + x86_64)"
MACOS_DIR="$BUILD_DIR/install-macos-fat"
mkdir -p "$MACOS_DIR/$FRAMEWORK.framework/Headers"
"$LIPO" -create \
    "$BUILD_DIR/install-macos-arm64/$FRAMEWORK.framework/$FRAMEWORK" \
    "$BUILD_DIR/install-macos-x86_64/$FRAMEWORK.framework/$FRAMEWORK" \
    -output "$MACOS_DIR/$FRAMEWORK.framework/$FRAMEWORK"
cp -r "$BUILD_DIR/install-iphoneos-arm64/$FRAMEWORK.framework/Headers/." \
    "$MACOS_DIR/$FRAMEWORK.framework/Headers/"
cp "$BUILD_DIR/install-macos-arm64/$FRAMEWORK.framework/Info.plist" \
    "$MACOS_DIR/$FRAMEWORK.framework/"

# ---------------------------------------------------------------------------
# Assemble xcframework
# ---------------------------------------------------------------------------

echo "==> Creating $FRAMEWORK.xcframework"
rm -rf "$OUTPUT"
xcodebuild -create-xcframework \
    -framework "$BUILD_DIR/install-iphoneos-arm64/$FRAMEWORK.framework" \
    -framework "$SIM_DIR/$FRAMEWORK.framework" \
    -framework "$MACOS_DIR/$FRAMEWORK.framework" \
    -output "$OUTPUT"

rm -rf "$BUILD_DIR"

echo ""
echo "Done! $OUTPUT"
echo "  Slices: $(ls "$OUTPUT" | grep -v Info.plist | tr '\n' ' ')"
