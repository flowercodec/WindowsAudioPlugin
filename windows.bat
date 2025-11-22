@echo off
setlocal

cd /d %~dp0

set CONFIG=Release

REM ==== Win32 (optional; disabled by default) ====
REM if not exist build\win32 mkdir build\win32
REM cd build\win32
REM cmake -A Win32 ..\..
REM if errorlevel 1 goto :cmake_fail
REM cmake --build . --config %CONFIG%
REM if errorlevel 1 goto :build_fail
REM cd ..\..

REM ==== Win64 only ====
if not exist build\win64 mkdir build\win64
cd build\win64
cmake -A x64 ..\..
if errorlevel 1 goto :cmake_fail
REM cmake --build . --config %CONFIG%
REM if errorlevel 1 goto :build_fail
cd ..\..

echo.
echo Build success.
goto :eof

:cmake_fail
echo CMake configure failed.
goto :eof

:build_fail
echo Build failed.
goto :eof


