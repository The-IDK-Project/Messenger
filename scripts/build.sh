#!/bin/bash

set -e

BUILD_TYPE="Release"
BUILD_DIR="build"
ENABLE_GUI="OFF"
ENABLE_TELEGRAM="OFF"
ENABLE_TESTS="OFF"

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --gui)
            ENABLE_GUI="ON"
            shift
            ;;
        --telegram)
            ENABLE_TELEGRAM="ON"
            shift
            ;;
        --tests)
            ENABLE_TESTS="ON"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--debug] [--gui] [--telegram] [--tests]"
            exit 1
            ;;
    esac
done

echo "Building Unified Messenger..."
echo "Build type: $BUILD_TYPE"
echo "GUI: $ENABLE_GUI"
echo "Telegram: $ENABLE_TELEGRAM"
echo "Tests: $ENABLE_TESTS"

mkdir -p $BUILD_DIR
cd $BUILD_DIR

cmake .. \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DBUILD_GUI=$ENABLE_GUI \
    -DENABLE_TELEGRAM=$ENABLE_TELEGRAM \
    -DBUILD_TESTS=$ENABLE_TESTS

if command -v nproc >/dev/null 2>&1; then
    CORES=$(nproc)
else
    CORES=4
fi

echo "Building with $CORES cores..."
make -j$CORES

echo "Build completed successfully!"
echo "Binary: $BUILD_DIR/unified-messenger"