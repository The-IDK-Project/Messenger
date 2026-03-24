@echo off
setlocal

REM -- Script to build the UnifiedMessenger project using Meson on Windows --

REM -- Build directory --
set BUILD_DIR=build

REM -- Check for Meson --
where meson >nul 2>nul
if %errorlevel% neq 0 (
    echo "Meson not found. Please install Meson and add it to your PATH."
    echo "https://mesonbuild.com/Getting-meson.html"
    exit /b 1
)

REM -- Check for Ninja (or other backend) --
where ninja >nul 2>nul
if %errorlevel% neq 0 (
    echo "Ninja not found. Make sure you have a compiler (e.g., MSVC) and Ninja installed."
)

REM -- Create build directory --
if not exist %BUILD_DIR% (
    echo "Creating build directory: %BUILD_DIR%"
    mkdir %BUILD_DIR%
)

REM -- Configure Meson --
echo "Configuring the project..."
meson setup %BUILD_DIR% .
if %errorlevel% neq 0 (
    echo "Meson configuration failed."
    exit /b 1
)

REM -- Build the project --
echo "Building the project..."
meson compile -C %BUILD_DIR%
if %errorlevel% neq 0 (
    echo "Build failed."
    exit /b 1
)

echo "Build successful!"
echo "Executables are in the %BUILD_DIR% directory."

endlocal
