#!/bin/bash
set -e

# Get the project root directory (parent of scripts directory)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Default to Google style
STYLE="Google"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --style=*)
            STYLE="${1#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Change to project root directory
cd "$PROJECT_ROOT"

# First do a dry run to check which files need formatting
echo "Checking files..."
fd -e cpp -e h -e hpp -0 | while IFS= read -r -d '' file; do
    if ! clang-format --style=${STYLE} --dry-run -Werror "${file}" &>/dev/null; then
        echo "Will format: ${file}"
    else
        echo "Already formatted: ${file}"
    fi
done

# Now do the actual formatting
echo -e "\nFormatting files..."
fd -e cpp -e h -e hpp -0 | xargs -0 clang-format -i --style=${STYLE}

echo -e "\nAll C++ files processed using ${STYLE} style" 