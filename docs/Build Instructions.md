# Build Instructions
## Dependencies
```
# Ubuntu/Debian
sudo apt install build-essential cmake pkg-config \
libcurl4-openssl-dev libjsoncpp-dev sqlite3 libsqlite3-dev \
libncurses5-dev libncursesw5-dev

# macOS
brew install cmake pkg-config curl jsoncpp sqlite3 ncurses

# Fedora
sudo dnf install gcc-c++ cmake pkgconfig \
libcurl-devel jsoncpp-devel sqlite-devel ncurses-devel
```
## Build
```
git clone https://github.com/The-IDK-Project/Messenger
cd Messenger
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
## Optional Features
```
# With GUI (Qt)
cmake .. -DBUILD_GUI=ON

# With Telegram support
cmake .. -DENABLE_TELEGRAM=ON

# Debug build with tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```
## Install
```
sudo make install
```
## Run
```
./Messenger
```
## Test
```
cd build
ctest  # Run all tests
./tests/unit/test_database  # Specific test
```
