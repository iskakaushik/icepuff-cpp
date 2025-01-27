#!/bin/bash
set -e

# Get the project root directory (parent of scripts directory)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Build the project with tests enabled
"$SCRIPT_DIR/build.sh" --debug

# Change to project root directory
cd "$PROJECT_ROOT"

# Run the tests
cd build && ctest --output-on-failure 