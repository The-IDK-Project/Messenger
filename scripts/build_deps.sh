#!/bin/bash

set -e

echo "Building third-party dependencies..."

THIRD_PARTY_DIR="third_party"
BUILD_DIR="build_third_party"
mkdir -p $BUILD_DIR

if [ -d "$THIRD_PARTY_DIR/sqlite3" ]; then
    echo "Building SQLite3..."
    cd $THIRD_PARTY_DIR/sqlite3
    ./configure --disable-shared --enable-static --prefix=$(pwd)/../../$BUILD_DIR
    make -j$(nproc)
    make install
    cd ../..
else
    echo "SQLite3 not found, skipping..."
fi

if [ -d "$THIRD_PARTY_DIR/tdlib" ] && [ "$1" = "--with-telegram" ]; then
    echo "Building TDLib..."
    cd $THIRD_PARTY_DIR/tdlib
    mkdir -p build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=$(pwd)/../../../$BUILD_DIR \
          ..
    make -j$(nproc)
    make install
    cd ../../..
else
    if [ "$1" != "--with-telegram" ]; then
        echo "Skipping TDLib build (use --with-telegram to include)"
    else
        echo "TDLib not found, skipping..."
    fi
fi

echo "Dependencies built successfully!"