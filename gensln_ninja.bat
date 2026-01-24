@echo off
setlocal

set BUILD_DIR=Local\ninja

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

(
echo CompileFlags:
echo   CompilationDatabase: Local/ninja
) > .clangd

pushd "%BUILD_DIR%"

cmake ..\.. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL_SHARED=ON -DSDL_STATIC=OFF
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
