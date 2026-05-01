#!/bin/bash

# Exit on error
set -e

# Linux build
echo "Building for Linux..."
meson setup build_linux --buildtype=release
meson compile -C build_linux

# Windows cross-build
echo "Building for Windows..."
meson setup build_windows --cross-file cross-file-mingw.txt --buildtype=release
meson compile -C build_windows

# Create distribution packages
echo "Creating distribution packages..."
DIST_DIR="dist"
rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR/linux"
mkdir -p "$DIST_DIR/windows"

# Linux package
cp build_linux/VeritaHeadless "$DIST_DIR/linux/"
cp build_linux/UnifiedMessenger_GUI "$DIST_DIR/linux/"
tar -czf "$DIST_DIR/verita-linux.tar.gz" -C "$DIST_DIR/linux" .

# Windows package
cp build_windows/VeritaHeadless.exe "$DIST_DIR/windows/"
cp build_windows/UnifiedMessenger_GUI.exe "$DIST_DIR/windows/"

# Find and copy MinGW DLLs
# This is a bit of a hack, a better solution would be to use a proper installer generator
DLL_PATH=$(dirname $(which x86_64-w64-mingw32-gcc))
cp $DLL_PATH/libgcc_s_seh-1.dll "$DIST_DIR/windows/"
cp $DLL_PATH/libstdc++-6.dll "$DIST_DIR/windows/"
cp $DLL_PATH/libwinpthread-1.dll "$DIST_DIR/windows/"

# Find and copy Qt DLLs (assuming they are in the pkg-config path)
QT_DLLS=$(pkg-config --variable=prefix Qt6Core)
cp $QT_DLLS/bin/Qt6Core.dll "$DIST_DIR/windows/"
cp $QT_DLLS/bin/Qt6Gui.dll "$DIST_DIR/windows/"
cp $QT_DLLS/bin/Qt6Widgets.dll "$DIST_DIR/windows/"
cp $QT_DLLS/bin/Qt6Network.dll "$DIST_DIR/windows/"
# Copy Qt platform plugin
mkdir -p "$DIST_DIR/windows/platforms"
cp $QT_DLLS/plugins/platforms/qwindows.dll "$DIST_DIR/windows/platforms/"


zip -r "$DIST_DIR/verita-windows.zip" "$DIST_DIR/windows"

echo "Build finished. Distribution packages are in the '$DIST_DIR' directory."
