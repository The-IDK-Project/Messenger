@echo off
setlocal

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
  echo vswhere.exe not found. Install Visual Studio Build Tools or run from Developer PowerShell.
  exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find Common7\Tools\VsDevCmd.bat`) do set "VSDEVCMD=%%i"

if not defined VSDEVCMD (
  echo VsDevCmd.bat not found. Install MSVC build tools.
  exit /b 1
)

call "%VSDEVCMD%" -host_arch=x64 -arch=x64
meson setup buildDir --reconfigure
