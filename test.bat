@echo off
setlocal enabledelayedexpansion

set "vswhere=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "vcvars="
if exist "%vswhere%" (
    for /f "usebackq tokens=*" %%i in (`"%vswhere%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "install_path=%%i"
)
if defined install_path set "vcvars=!install_path!\VC\Auxiliary\Build\vcvars64.bat"
if defined vcvars if exist "!vcvars!" call "!vcvars!" >nul

where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] MSVC compiler not found
    exit /b 1
)

if not exist bin mkdir bin
cl /nologo /utf-8 /EHsc /std:c++17 tests\vmd_core_tests.cpp /Fe"bin\vmd_core_tests.exe"
if %errorlevel% neq 0 exit /b 1
bin\vmd_core_tests.exe
exit /b %errorlevel%
