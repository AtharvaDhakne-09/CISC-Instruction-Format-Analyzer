@echo off
echo Building CISC Instruction Format Analyzer...
cd /d "%~dp0"
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
if %errorlevel% neq 0 (
    echo Build failed.
    pause
    exit /b 1
)
echo.
echo Starting server...
echo Open http://localhost:8080 in Chrome or Edge.
echo Press Ctrl+C to stop the server.
echo.
cd /d "%~dp0"
.\build\cisc_instruction_analyzer.exe

