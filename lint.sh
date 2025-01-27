#!/bin/bash
set -e

# Find clang-tidy binary
if command -v clang-tidy &>/dev/null; then
    CLANG_TIDY="clang-tidy"
elif command -v brew &>/dev/null && [ -f "$(brew --prefix llvm)/bin/clang-tidy" ]; then
    CLANG_TIDY="$(brew --prefix llvm)/bin/clang-tidy"
else
    echo "Error: clang-tidy not found. Please install it via 'brew install llvm' or your system's package manager."
    exit 1
fi

# Default checks
CHECKS="\
modernize*,\
performance*,\
readability*,\
bugprone*,\
clang-analyzer*,\
cppcoreguidelines*,\
-modernize-use-trailing-return-type"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --checks=*)
            CHECKS="${1#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Ensure compile_commands.json exists
if [ ! -f "compile_commands.json" ]; then
    echo "Error: compile_commands.json not found."
    echo "Please run build.sh first to generate it."
    exit 1
fi

# Get project root directory
PROJECT_ROOT="$(pwd)"

# Run clang-tidy
echo "Running clang-tidy..."
fd -e cpp -0 | while IFS= read -r -d '' file; do
    echo "Checking: ${file}"
    if ! "${CLANG_TIDY}" -p . --checks=${CHECKS} "${file}" -- \
        -std=c++17 \
        -I"${PROJECT_ROOT}/include" \
        -I"${PROJECT_ROOT}/build/_deps/fmt-src/include"; then
        echo "Found issues in: ${file}"
    else
        echo "No issues found in: ${file}"
    fi
done

echo -e "\nLinting complete" 