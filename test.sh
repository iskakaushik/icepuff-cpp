#!/bin/bash
set -e

# Build the project with tests enabled
./build.sh --debug

# Run the tests
cd build && ctest --output-on-failure 