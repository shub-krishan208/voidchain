#!/bin/bash

mode="debug"

echo "Building VoidChain [DEBUG] ..."
cmake --preset debug .
cmake --build --preset $mode

echo "Testing ..."
ctest --preset $mode
if [ $? -ne 0 ]; then
  echo "Tests failed!"
  exit 1
  else
  echo "All tests passed!"
fi
echo "Running VoidChain ..."
./build/$mode/voidchain
