@echo off
setlocal

set BUILD_DIR=Local\sln

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

pushd "%BUILD_DIR%"

cmake ..\.. -G "Visual Studio 18" -DSDL_SHARED=ON -DSDL_STATIC=OFF -DCMAKE_CONFIGURATION_TYPES="Debug;Release;RelWithDebInfo"
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    popd
    exit /b 1
)

popd

echo.
echo Visual Studio solution generated successfully!
echo Open %BUILD_DIR%\Empire.sln to build and run.
