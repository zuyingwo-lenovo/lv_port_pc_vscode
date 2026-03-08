@echo off
setlocal

echo =========================================
echo Building lv_port_pc_vscode
echo =========================================

:: Check if build directory exists, if not create it
if not exist "build" (
    mkdir build
)

cd build

:: Configure the project
echo.
echo [1/2] Configuring CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE="E:\vcpkg\scripts\buildsystems\vcpkg.cmake" -DHailoRT_DIR="C:\Program Files\HailoRT\lib\cmake\HailoRT"
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: CMake configuration failed.
    echo Please ensure you have CMake and an appropriate compiler installed.
    echo If you haven't installed SDL2 yet on Windows, consider using vcpkg:
    echo vcpkg install sdl2
    exit /b %ERRORLEVEL%
)

:: Build the project
echo.
echo [2/2] Building project...
cmake --build . --parallel
if %ERRORLEVEL% neq 0 (
    echo.
    echo ERROR: Build failed.
    exit /b %ERRORLEVEL%
)

echo.
echo =========================================
echo Build completed successfully!
echo The executable should be located in the \bin directory.
echo =========================================

endlocal
