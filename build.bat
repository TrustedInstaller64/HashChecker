@echo off
rem -- Windows build script (Ninja + MSVC) --

set "VCDEVCMD="
if exist "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCDEVCMD=D:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set "VCDEVCMD=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
)

if "%VCDEVCMD%"=="" (
    echo [ERROR] Visual Studio not found.
    pause
    exit /b 1
)
call "%VCDEVCMD%"

where ninja >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Ninja not found. Install: winget install Ninja-build.Ninja
    pause
    exit /b 1
)

if exist build rmdir /s /q build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release %*
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
cmake --build build --parallel
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo === Build complete ===
echo.
set /p RUN_TEST="Run end-to-end test? (Y/N): "
if /i "%RUN_TEST%"=="Y" (
    echo.
    tests\scripts\test_e2e.cmd
) else (
    pause
)
