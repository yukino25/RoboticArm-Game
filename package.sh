#!/bin/bash
# package.sh — builds and packages Robotic Arm Game into a distributable .zip
set -e

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR="$REPO_ROOT/Game"
BUILD_DIR="$GAME_DIR/build"
APP_NAME="RoboticArmGame"
APP="$REPO_ROOT/$APP_NAME.app"
MACOS="$APP/Contents/MacOS"
ZIP="$REPO_ROOT/$APP_NAME.zip"

echo "==> Building Release..."
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=ON 2>/dev/null || true
cmake --build . --config Release -j"$(sysctl -n hw.logicalcpu)"

BINARY="$BUILD_DIR/game"
SDL3_DYLIB="$(find "$BUILD_DIR/_deps" -name "libSDL3.0.dylib"    | head -1)"
SDL3I_DYLIB="$(find "$BUILD_DIR/_deps" -name "libSDL3_image.0.dylib" | head -1)"

if [ ! -f "$BINARY" ]; then  echo "ERROR: build/game not found"; exit 1; fi
if [ ! -f "$SDL3_DYLIB" ];  then echo "ERROR: libSDL3.0.dylib not found";       exit 1; fi
if [ ! -f "$SDL3I_DYLIB" ]; then echo "ERROR: libSDL3_image.0.dylib not found"; exit 1; fi

echo "==> Creating app bundle..."
rm -rf "$APP"
mkdir -p "$MACOS"

# Info.plist
cat > "$APP/Contents/Info.plist" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
    "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>   <string>launch</string>
    <key>CFBundleIdentifier</key>  <string>com.example.robotic-arm</string>
    <key>CFBundleName</key>        <string>Robotic Arm Game</string>
    <key>CFBundlePackageType</key> <string>APPL</string>
    <key>CFBundleVersion</key>     <string>0.1</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
EOF

# Launcher shell script — sets working dir so relative asset paths work
cat > "$MACOS/launch" << 'LAUNCH'
#!/bin/bash
cd "$(dirname "$0")"
exec ./game
LAUNCH
chmod +x "$MACOS/launch"

# Copy binary and dylibs
cp "$BINARY"      "$MACOS/game"
cp "$SDL3_DYLIB"  "$MACOS/libSDL3.0.dylib"
cp "$SDL3I_DYLIB" "$MACOS/libSDL3_image.0.dylib"

# Fix rpath so game binary finds dylibs next to itself
install_name_tool -add_rpath @executable_path/ "$MACOS/game"

# Copy game assets and levels
cp -r "$GAME_DIR/assets" "$MACOS/assets"
cp -r "$GAME_DIR/levels" "$MACOS/levels"

echo "==> Packaging into zip..."
rm -f "$ZIP"
cd "$REPO_ROOT"
zip -r "$ZIP" "$APP_NAME.app" --exclude "*.DS_Store"

echo ""
echo "Done: $ZIP"
echo "  Send this zip — recipient double-clicks $APP_NAME.app to play."
echo ""
echo "NOTE: macOS Gatekeeper will block unsigned apps. Recipient may need to"
echo "  right-click → Open the first time, or run:"
echo "  xattr -cr $APP_NAME.app"
