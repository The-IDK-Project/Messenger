# Unified Messenger 🚀

[English](#english) | [Русский](#русский)

<a name="english"></a>
# Unified Messenger - Multi-Protocol Chat Client

![C++](https://img.shields.io/badge/C++-17+-blue.svg)
![CMake](https://img.shields.io/badge/CMake-3.15+-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)
![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)

A modern, unified chat client that brings together Matrix, IRC, and Telegram in one seamless interface. Built with C++ for performance and reliability.

## ✨ Features

- **Multi-Protocol Support**: Connect to Matrix, IRC, and Telegram simultaneously
- **Unified Inbox**: View all messages from different protocols in one place
- **Cross-Platform**: Runs on Linux, macOS, and Windows
- **SQLite Database**: Local message history and user data storage
- **Multiple UI Options**: Text-based (TUI) and Graphical (GUI) interfaces
- **Real-time Sync**: Live updates from all connected protocols
- **End-to-End Encryption**: Secure communications where supported
- **Customizable**: Themes, keybindings, and protocol-specific settings

## 🚀 Quick Start

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake pkg-config \
    libcurl4-openssl-dev libjsoncpp-dev sqlite3 libsqlite3-dev \
    libncurses5-dev libncursesw5-dev

# macOS
brew install cmake pkg-config curl jsoncpp sqlite3 ncurses

# Fedora
sudo dnf install gcc-c++ cmake pkgconfig \
    libcurl-devel jsoncpp-devel sqlite-devel ncurses-devel
```

### Build & Install

```
git clone https://github.com/The-IDK-Project/super-duper-giggle.git
cd super-duper-giggle
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install  
```
### Run
```
./super-duper-giggle
```

### Documentation

[Architecture Overview](https://github.com/The-IDK-Project/Messenger/blob/master/docs/ARCHITECTURE.md)

[Build Instructions](https://github.com/The-IDK-Project/Messenger/blob/master/docs/Build%20Instructions.md)

[Contributing Guide](https://github.com/The-IDK-Project/super-duper-giggle/blob/main/docs/Contributing%20Guide.md)

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

[telegram]
api_id = your_api_id
api_hash = your_api_hash
Protocol Setup
Matrix: Get access token from Element client settings
IRC: Just provide server and nickname
Telegram: Get API credentials from https://my.telegram.org
```

# Project Structure
```
unified-messenger/
├── include/                 # Header files
├── src/                    # Source code
├── tests/                  # Unit and integration tests
├── config/                 # Configuration files
├── docs/                   # Documentation
└── third_party/            # External dependencies
```
# Advanced Features
### Build Options
```
# With GUI (Qt)
cmake .. -DBUILD_GUI=ON

# With Telegram support (TDLib)
cmake .. -DENABLE_TELEGRAM=ON

# Debug build with tests
```
```
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
Database Management
Messages are stored in SQLite at ~/.local/share/unified-messenger/messages.db
```

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

    Telegram for their API

    TDLib for Telegram integration

    All our wonderful contributors
