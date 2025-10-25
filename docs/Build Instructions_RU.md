# Инструкции по сборке
## Зависимости
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
## Сборка
```
git clone https://github.com/yourusername/unified-messenger
cd unified-messenger
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
## Опциональные функции
```
# With GUI (Qt)
cmake .. -DBUILD_GUI=ON

# With Telegram support
cmake .. -DENABLE_TELEGRAM=ON

# Debug build with tests
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```
## Установка
```
sudo make install
```
## Запуск
```
./unified-messenger
```
## Тестирование
```
cd build
ctest  # Run all tests
./tests/unit/test_database  # Specific test
```