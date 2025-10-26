#!/bin/bash

set -e

echo "Creating AppImage for Unified Messenger..."

# Create AppDir structure
mkdir -p AppDir/usr/bin
mkdir -p AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons/hicolor/256x256/apps

# Copy binary
cp build/unified-messenger AppDir/usr/bin/

# Copy desktop file
cat > AppDir/usr/share/applications/unified-messenger.desktop << EOF
[Desktop Entry]
Name=Unified Messenger
Comment=Multi-protocol chat client
Exec=unified-messenger
Icon=unified-messenger
Terminal=false
Type=Application
Categories=Network;InstantMessaging;
StartupWMClass=Unified Messenger
EOF

# Copy icon (if exists)
if [ -f resources/icons/app_icon.png ]; then
    cp resources/icons/app_icon.png AppDir/usr/share/icons/hicolor/256x256/apps/unified-messenger.png
fi

# Download linuxdeploy
wget -q -O linuxdeploy-x86_64.AppImage https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
chmod +x linuxdeploy-x86_64.AppImage

# Create AppImage
./linuxdeploy-x86_64.AppImage \
    --appdir AppDir \
    --output appimage

echo "AppImage created successfully!"
