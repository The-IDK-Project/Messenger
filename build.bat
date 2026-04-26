@echo off
setlocal

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%buildDir"

call :stop_build_tools

where qmake6 >nul 2>nul
if errorlevel 1 where qmake >nul 2>nul
if errorlevel 1 (
  echo qmake was not found in PATH. Install Qt and expose qmake6/qmake.
  exit /b 1
)

for /f "usebackq delims=" %%i in (`where qmake6 2^>nul`) do (
  set "QMAKE=%%i"
  goto :qmake_found
)
for /f "usebackq delims=" %%i in (`where qmake 2^>nul`) do (
  set "QMAKE=%%i"
  goto :qmake_found
)

:qmake_found
for /f "usebackq delims=" %%i in (`"%QMAKE%" -query QMAKE_SPEC`) do set "QMAKE_SPEC=%%i"
for /f "usebackq delims=" %%i in (`"%QMAKE%" -query QT_INSTALL_BINS`) do set "QT_BIN_DIR=%%i"

echo Using Qt from: %QT_BIN_DIR%
echo Qt spec: %QMAKE_SPEC%

echo %QMAKE_SPEC% | findstr /c:"g++" >nul
if not errorlevel 1 goto :build_with_mingw

call :setup_msvc
if errorlevel 1 exit /b 1
meson setup "%BUILD_DIR%" --reconfigure
if errorlevel 1 exit /b 1
meson compile -C "%BUILD_DIR%"
exit /b %errorlevel%

:stop_build_tools
for %%p in (meson.exe ninja.exe gcc.exe g++.exe ld.exe ar.exe windres.exe) do (
  taskkill /f /im %%p >nul 2>nul
)
exit /b 0

:build_with_mingw
call :setup_mingw
if errorlevel 1 exit /b 1
meson setup "%BUILD_DIR%" --wipe
if errorlevel 1 exit /b 1
meson compile -C "%BUILD_DIR%"
exit /b %errorlevel%

:setup_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo vswhere.exe not found. Install Visual Studio Build Tools or use a MinGW Qt build.
  exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat`) do set "VSDEVCMD=%%i"
if not defined VSDEVCMD (
  echo VsDevCmd.bat not found. Install MSVC build tools.
  exit /b 1
)

call "%VSDEVCMD%" -host_arch=x64 -arch=x64
exit /b %errorlevel%

:setup_mingw
set "MINGW_BIN="

for %%d in (
  "C:\Program Files\JetBrains\CLion 2026.1\bin\mingw\bin"
  "C:\Program Files\JetBrains\CLion\bin\mingw\bin"
  "C:\Qt\Tools\mingw1310_64\bin"
  "C:\Qt\Tools\mingw1120_64\bin"
  "C:\Q\Tools\mingw1310_64\bin"
  "C:\Q\Tools\mingw1120_64\bin"
) do (
  if exist %%~d\g++.exe (
    set "MINGW_BIN=%%~d"
    goto :mingw_found
  )
)

echo MinGW compiler was not found. Install Qt MinGW tools or CLion bundled MinGW.
exit /b 1

:mingw_found
set "PATH=%MINGW_BIN%;%QT_BIN_DIR%;%PATH%"
set "CC=gcc"
set "CXX=g++"
echo Using MinGW from: %MINGW_BIN%
exit /b 0
