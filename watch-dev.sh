#!/bin/bash

BIN=./build/debug/voidchain

if ! command -v entr &> /dev/null; then
    echo "Please install 'entr' to use watch mode."
    exit 1
fi

# if ! command -v vcpkg &> /dev/null; then
#     echo "Please install 'vcpkg' to use watch mode."
#     exit 1
# fi
    

cmake --preset debug .

echo "Starting Watch mode..."
find src include tests -name "*.*" | entr -r bash -c "
    cmake --build --preset debug && 
    ctest --preset debug --output-on-failure &&
    exec ${BIN}"
