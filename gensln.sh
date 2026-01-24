#!/bin/bash

BUILD_DIR="Local/ninja"

mkdir -p "$BUILD_DIR"

cat > .clangd << EOF
CompileFlags:
  CompilationDatabase: $BUILD_DIR
EOF

cd "$BUILD_DIR"

cmake ../.. -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DSDL_SHARED=ON -DSDL_STATIC=OFF
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

cd ../..

echo ""
echo "Project generated successfully!"
echo "Build files are in $BUILD_DIR/"
echo "compile_commands.json is at $BUILD_DIR/compile_commands.json"
echo "Run 'ninja -C $BUILD_DIR' to build."
