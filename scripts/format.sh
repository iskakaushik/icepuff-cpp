#!/bin/bash
set -e

# Get the project root directory (parent of scripts directory)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Default to Google style
STYLE="Google"
CHECK_ONLY=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --style=*)
            STYLE="${1#*=}"
            shift
            ;;
        --check)
            CHECK_ONLY=true
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

# Variable to track if any files need formatting
NEEDS_FORMATTING=false

# Check which files need formatting
echo "Checking files..."
fd -e cpp -e h -e hpp -0 | while IFS= read -r -d '' file; do
    if ! clang-format --style=${STYLE} --dry-run -Werror "${file}" &>/dev/null; then
        echo "Needs formatting: ${file}"
        NEEDS_FORMATTING=true
    else
        echo "Correctly formatted: ${file}"
    fi
done

if [ "$CHECK_ONLY" = true ]; then
    if [ "$NEEDS_FORMATTING" = true ]; then
        echo -e "\nSome files need formatting. Run without --check to format them."
        exit 1
    else
        echo -e "\nAll files are correctly formatted."
        exit 0
    fi
fi

# Do the actual formatting
echo -e "\nFormatting files..."
fd -e cpp -e h -e hpp -0 | xargs -0 clang-format -i --style=${STYLE}

echo -e "\nAll C++ files processed using ${STYLE} style" 