@echo off
echo === Cloud Task Orchestration Engine - Windows Build ===

where cmake >nul 2>&1 || (echo ERROR: cmake not found. && exit /b 1)

set BUILD_TYPE=Release
if not "%1"=="" set BUILD_TYPE=%1

echo [1/3] Configuring...
cmake -S . -B build -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

echo [2/3] Building...
cmake --build build --config %BUILD_TYPE% --parallel

echo [3/3] Done!
echo.
echo Run with:  build\%BUILD_TYPE%\orchestrator.exe
