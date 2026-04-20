#!/bin/bash
# package_windows.sh — cross-compiles for Windows (x64) from macOS and zips a distributable folder.
set -e

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR="$REPO_ROOT/Game"
WORK_DIR="$REPO_ROOT/_win_build"
OUT_NAME="RoboticArmGame_Windows"
OUT_DIR="$REPO_ROOT/$OUT_NAME"
ZIP="$REPO_ROOT/$OUT_NAME.zip"

SDL3_VER="3.2.10"
SDL3I_VER="3.2.4"
SDL3_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VER}/SDL3-devel-${SDL3_VER}-mingw.zip"
SDL3I_URL="https://github.com/libsdl-org/SDL_image/releases/download/release-${SDL3I_VER}/SDL3_image-devel-${SDL3I_VER}-mingw.zip"

# ── Prerequisites ─────────────────────────────────────────────────────────────
if ! command -v x86_64-w64-mingw32-g++ &>/dev/null; then
    echo "==> Installing mingw-w64..."
    brew install mingw-w64
fi

mkdir -p "$WORK_DIR"

# ── Download SDL3 dev package ─────────────────────────────────────────────────
SDL3_EXTRACT="$WORK_DIR/SDL3-${SDL3_VER}"
if [ ! -d "$SDL3_EXTRACT" ]; then
    echo "==> Downloading SDL3 ${SDL3_VER} (mingw)..."
    curl -L --progress-bar "$SDL3_URL" -o "$WORK_DIR/SDL3-mingw.zip"
    unzip -q "$WORK_DIR/SDL3-mingw.zip" -d "$WORK_DIR"
fi

# ── Download SDL3_image dev package ──────────────────────────────────────────
SDL3I_EXTRACT="$WORK_DIR/SDL3_image-${SDL3I_VER}"
if [ ! -d "$SDL3I_EXTRACT" ]; then
    echo "==> Downloading SDL3_image ${SDL3I_VER} (mingw)..."
    curl -L --progress-bar "$SDL3I_URL" -o "$WORK_DIR/SDL3_image-mingw.zip"
    unzip -q "$WORK_DIR/SDL3_image-mingw.zip" -d "$WORK_DIR"
fi

SDL3_X64="$SDL3_EXTRACT/x86_64-w64-mingw32"
SDL3I_X64="$SDL3I_EXTRACT/x86_64-w64-mingw32"

# ── Cross-compile ─────────────────────────────────────────────────────────────
echo "==> Compiling for Windows x64..."
cd "$GAME_DIR"
MINGW_LIB="/opt/homebrew/Cellar/mingw-w64/14.0.0/toolchain-x86_64/x86_64-w64-mingw32/lib"

x86_64-w64-mingw32-g++ -std=c++17 -O2 \
    -DSDL_MAIN_USE_CALLBACKS \
    -I"$SDL3_X64/include" \
    -I"$SDL3I_X64/include" \
    main.cpp gravity.cpp movement.cpp level.cpp render.cpp \
    -L"$SDL3_X64/lib" -L"$SDL3I_X64/lib" -L"$MINGW_LIB" \
    -lSDL3 -lSDL3_image \
    -static-libgcc -static-libstdc++ \
    -Wl,-Bstatic -lwinpthread -Wl,-Bdynamic \
    -mwindows \
    -o "$WORK_DIR/game.exe"

# ── Package ───────────────────────────────────────────────────────────────────
echo "==> Packaging..."
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

cp "$WORK_DIR/game.exe"              "$OUT_DIR/"
cp "$SDL3_X64/bin/SDL3.dll"          "$OUT_DIR/"
cp "$SDL3I_X64/bin/SDL3_image.dll"   "$OUT_DIR/"
cp -r "$GAME_DIR/assets"             "$OUT_DIR/assets"
cp -r "$GAME_DIR/levels"             "$OUT_DIR/levels"

# ── Zip ───────────────────────────────────────────────────────────────────────
rm -f "$ZIP"
cd "$REPO_ROOT"
zip -r "$ZIP" "$OUT_NAME" --exclude "*.DS_Store"

echo ""
echo "Done: $ZIP"
echo "  Send this zip — recipient extracts and runs game.exe"
echo "  (SDL3.dll and SDL3_image.dll must stay in the same folder as game.exe)"
