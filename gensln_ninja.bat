@echo off
setlocal

set BUILD_DIR=Local\ninja
set STATIC_BUILD=OFF
set TARGET_ARCH=

:parse_args
if "%~1"=="" goto done_args
if "%~1"=="-static" (
    set STATIC_BUILD=ON
    shift
    goto parse_args
)
if "%~1"=="-x64" (
    set TARGET_ARCH=x64
    shift
    goto parse_args
)
if "%~1"=="-arm64" (
    set TARGET_ARCH=arm64
    shift
    goto parse_args
)
shift
goto parse_args
:done_args

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

(
echo CompileFlags:
echo   CompilationDatabase: Local/ninja
) > .clangd

pushd "%BUILD_DIR%"

set CMAKE_EXTRA_ARGS=
if "%TARGET_ARCH%"=="x64" (
    echo Cross-compiling for x64...
    set CMAKE_EXTRA_ARGS=-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=x86_64-pc-windows-msvc -DCMAKE_SYSTEM_PROCESSOR=AMD64
)
if "%TARGET_ARCH%"=="arm64" (
    echo Cross-compiling for ARM64...
    set CMAKE_EXTRA_ARGS=-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER_TARGET=aarch64-pc-windows-msvc -DCMAKE_CXX_COMPILER_TARGET=aarch64-pc-windows-msvc -DCMAKE_SYSTEM_PROCESSOR=ARM64
)

if "%STATIC_BUILD%"=="ON" (
    echo Building with STATIC SDL...
    cmake ..\.. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL_SHARED=OFF -DSDL_STATIC=ON %CMAKE_EXTRA_ARGS%
) else (
    cmake ..\.. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL_SHARED=ON -DSDL_STATIC=OFF %CMAKE_EXTRA_ARGS%
)
if %ERRORLEVEL% neq 0 (
    echo CMake configuration failed!
    popd
    exit /b 1
)

popd

echo.
echo Project generated successfully!
echo Build files are in %BUILD_DIR%\
echo compile_commands.json is at %BUILD_DIR%\compile_commands.json
echo Run 'ninja -C %BUILD_DIR%' to build.
