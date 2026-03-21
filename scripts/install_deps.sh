#!/bin/bash

set -e

echo "Installing dependencies for Unified Messenger..."

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    if command -v apt-get >/dev/null 2>&1; then
        echo "Detected Debian/Ubuntu"
        sudo apt-get update
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libcurl4-openssl-dev \
            libjsoncpp-dev \
            sqlite3 \
            libsqlite3-dev \
            libncurses5-dev \
            libncursesw5-dev \
            git

    elif command -v dnf >/dev/null 2>&1; then
        echo "Detected Fedora/RHEL"
        sudo dnf install -y \
            gcc-c++ \
            cmake \
            pkgconfig \
            libcurl-devel \
            jsoncpp-devel \
            sqlite-devel \
            ncurses-devel \
            git

    elif command -v pacman >/dev/null 2>&1; then
        echo "Detected Arch Linux"
        sudo pacman -S --noconfirm \
            base-devel \
            cmake \
            pkg-config \
            curl \
            jsoncpp \
            sqlite \
            ncurses \
            git
    else
        echo "Unsupported Linux distribution"
        exit 1
    fi

elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Detected macOS"

    if ! command -v brew >/dev/null 2>&1; then
        echo "Homebrew not found. Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    brew install \
        cmake \
        pkg-config \
        curl \
        jsoncpp \
        sqlite3 \
        ncurses \
        git

else
    echo "Unsupported operating system: $OSTYPE"
    exit 1
fi

echo ""
echo "Optional dependencies:"
echo "- For GUI: Install Qt6 development packages"
echo "- For Telegram: Build TDLib from source"
echo "- For tests: Install gtest/gmock"

echo "Dependencies installed successfully!"