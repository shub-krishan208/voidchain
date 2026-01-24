#!/bin/bash

if ! [command -v entr &> /dev/null]; then
    echo "Please install 'entr' to use watch mode."
    exit 1
fi
    

if [! -d "build" ]; then
    echo "No "build" folder found. Creating one ..."
    cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake
fi

echo "Starting Watch mode..."
find src include tests -name "*.*" | entr -r bash -c "cmake --build build --parallel && ./build/voidchain"