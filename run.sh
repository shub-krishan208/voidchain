#!/bin/bash

cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=./vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ln -s build/compile_commands.json .

./build/voidchain
