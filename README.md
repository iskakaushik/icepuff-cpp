# IcyPuff C++

A C++ implementation of the [Puffin file format](https://iceberg.apache.org/puffin-spec) reader and writer. Currently supports local file operations.

## Overview

IcyPuff is a C++ library that provides functionality to read and write Puffin format files. The Puffin format is designed for efficient storage and retrieval of binary data with associated metadata.

## Features

- Local file reading and writing support
- Puffin format compliance
- Exception-free design
- Modern C++20 implementation
- Compression support (LZ4 and Zstd)

## Requirements

- CMake 3.15 or higher
- C++20 compatible compiler
- vcpkg package manager

## Dependencies

- fmt
- nlohmann-json
- lz4
- zstd
- spdlog
- gtest (for testing)
- cxxopts (for demo app)

## Demo Application

The project includes a demo application that showcases basic reading and writing operations with Puffin files. The demo app can:
- Write random inspirational quotes to a Puffin file with Zstd compression
- Read and display contents of a Puffin file, including file properties, blob metadata, and content

### Running the Demo

After building the project, you can find the demo app at `build/examples/demo/demo-icypuff`. Here's how to use it:

```bash
# Show help and available commands
./build/examples/demo/demo-icypuff -h

# Write a random quote to a Puffin file
./build/examples/demo/demo-icypuff -w output.puffin

# Read and display contents of a Puffin file
./build/examples/demo/demo-icypuff -r output.puffin
```

## Building

1. Install vcpkg if you haven't already:
```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
```

2. Clone the repository:
```bash
git clone https://github.com/iskakaushik/icypuff-cpp.git
cd icypuff-cpp
```

3. Build the project:
```bash
./scripts/build.sh
```

This will automatically install dependencies through vcpkg and build the project.

## Testing

Run the test suite:
```bash
./scripts/test.sh
```

## Contributing

1. Fork the repository
2. Create your feature branch
3. Make your changes
4. Ensure code quality:
   ```bash
   ./scripts/format.sh  # Format code
   ./scripts/lint.sh    # Run linter
   ./scripts/test.sh    # Run tests
   ```
5. Submit a pull request

### Code Style

- The project uses clang-format for code formatting
- Exception-free design (all error handling through Result types)
- Modern C++20 features are encouraged
- All new code must include tests
