#!/bin/bash

set -e

echo "Downloading third-party dependencies..."

THIRD_PARTY_DIR="third_party"
mkdir -p $THIRD_PARTY_DIR

if [ ! -d "$THIRD_PARTY_DIR/json" ]; then
    echo "Downloading nlohmann/json..."
    git clone https://github.com/nlohmann/json.git $THIRD_PARTY_DIR/json
    cd $THIRD_PARTY_DIR/json && git checkout v3.11.2 && cd ../..
else
    echo "nlohmann/json already exists"
fi

if [ ! -d "$THIRD_PARTY_DIR/sqlite3" ]; then
    echo "Downloading SQLite3..."
    SQLITE_VERSION="3420000"
    wget -q https://sqlite.org/2023/sqlite-autoconf-$SQLITE_VERSION.tar.gz
    tar -xf sqlite-autoconf-$SQLITE_VERSION.tar.gz
    mv sqlite-autoconf-$SQLITE_VERSION $THIRD_PARTY_DIR/sqlite3
    rm sqlite-autoconf-$SQLITE_VERSION.tar.gz
else
    echo "SQLite3 already exists"
fi

if [ ! -d "$THIRD_PARTY_DIR/tdlib" ] && [ "$1" = "--with-telegram" ]; then
    echo "Downloading TDLib..."
    git clone https://github.com/tdlib/td.git $THIRD_PARTY_DIR/tdlib
    cd $THIRD_PARTY_DIR/tdlib && git checkout v1.8.0 && cd ../..
else
    if [ "$1" != "--with-telegram" ]; then
        echo "Skipping TDLib (use --with-telegram to include)"
    else
        echo "TDLib already exists"
    fi
fi

echo "Dependencies downloaded successfully!"