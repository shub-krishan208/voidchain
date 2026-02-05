#!/bin/bash

mode="debug"

command -v vcpkg >/dev/null || {
  echo "Error: vcpkg not found"
  exit 1
}
command -v cmake >/dev/null || {
  echo "Error: cmake not found"
  exit 1
}
command -v ninja >/dev/null || {
  echo "Warning: ninja not found , falling back to default generator ..."
}

echo "Building VoidChain (${mode}) ..."
cmake --preset $mode .
cmake --build --preset $mode
