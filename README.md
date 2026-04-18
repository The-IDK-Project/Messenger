# Messenger - Multi-Protocol Chat Client

![C++](https://img.shields.io/badge/C++-20+-pink.svg)
![Meson](https://img.shields.io/badge/Meson-0.60+-purple.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)

A modern, unified chat client that brings together Matrix and IRC in one seamless interface. Built with C++ for performance and reliability.

## Features

- **Multi-Protocol Support**: Connect to Matrix and IRC simultaneously
- **Unified Inbox**: View all messages from different protocols in one place
- **Cross-Platform**: Runs on Linux, macOS, and Windows
- **SQLite Database**: Local message history and user data storage
- **Multiple UI Options**: Text-based (TUI) and Graphical (GUI) interfaces
- **Real-time Sync**: Live updates from all connected protocols
- **End-to-End Encryption**: Secure communications where supported
- **Customizable**: Themes, keybindings, and protocol-specific settings

## Quick Start

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential meson ninja-build pkg-config \
    libcurl4-openssl-dev libjsoncpp-dev sqlite3 libsqlite3-dev \
    libncurses5-dev libncursesw5-dev

# macOS
brew install meson ninja pkg-config curl jsoncpp sqlite3 ncurses

# Fedora
sudo dnf install gcc-c++ meson ninja-build pkgconfig \
    libcurl-devel jsoncpp-devel sqlite-devel ncurses-devel
```

### Build & Install

```
git clone https://github.com/The-IDK-Project/Messenger.git
cd Messenger
meson setup build
cd build
ninja
sudo ninja install
```
### Run
```
./unified-messenger
```

### Documentation

[Architecture Overview](https://github.com/The-IDK-Project/Messenger/blob/master/docs/ARCHITECTURE.md)

[Build Instructions](https://github.com/The-IDK-Project/Messenger/blob/master/docs/Build%20Instructions.md)

[Contributing Guide](https://github.com/The-IDK-Project/Messenger/blob/master/docs/Contributing%20Guide.md)

# Configuration
### Basic Configuration

Create ~/.config/unified-messenger/config.conf:

```
[ui]
theme = dark
notifications = true

[matrix]
homeserver = https://matrix.org
access_token = your_token

[irc]
server = irc.libera.chat
port = 6667
nickname = your_nick
```
### Protocol Setup
Matrix: Get access token from Element client settings
IRC: Just provide server and nickname

# Project Structure
```
unified-messenger/
├── include/                 # Header files
├── src/                    # Source code
├── tests/                  # Unit and integration tests
├── config/                 # Configuration files
├── docs/                   # Documentation
└── subprojects/            # External dependencies (via Meson wraps)
```
# Advanced Features
### Build Options
```
# With GUI (Qt)
meson setup build -Dbuild_gui=true

# Debug build with tests
meson setup build -Dbuildtype=debug -Dbuild_tests=true
```
Database Management
Messages are stored in SQLite at ~/.local/share/unified-messenger/messages.db

# Contributing
We welcome contributions! Please see our Contributing Guide for details.

    Fork the repository

    Create a feature branch (git checkout -b feature/amazing-feature)

    Commit your changes (git commit -m 'Add amazing feature')

    Push to the branch (git push origin feature/amazing-feature)

    Open a Pull Request

# License

This project is licensed under the MIT License - see the LICENSE file for details.

# Acknowledgments
    Matrix.org for the Matrix protocol

    Libera Chat for IRC services

    All our wonderful contributors

# Chat on matrix: #theidkteam:matrix.org

# Third-Party Dependencies

This directory contains third-party libraries used by Unified Messenger.

## Included Libraries

### nlohmann/json
- **Version**: 3.11.2
- **License**: MIT
- **Purpose**: JSON parsing and serialization
- **Usage**: Single-header library, no build required

### SQLite3
- **Version**: 3.42.0
- **License**: Public Domain
- **Purpose**: Embedded database engine
- **Usage**: Built as static library

## Management

Dependencies are managed via Meson's wrap system. See the `subprojects` directory.
