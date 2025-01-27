#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "icypuff/result.h"

namespace icypuff {

class SeekableInputStream {
 public:
  virtual ~SeekableInputStream() = default;

  // Read up to length bytes into the buffer starting at position
  virtual Result<size_t> read(uint8_t* buffer, size_t length) = 0;

  // Skip length bytes
  virtual Result<void> skip(int64_t length) = 0;

  // Seek to a specific position in the stream
  virtual Result<void> seek(int64_t position) = 0;

  // Get the current position in the stream
  virtual Result<int64_t> position() const = 0;

  // Close the stream
  virtual Result<void> close() = 0;
};

}  // namespace icypuff 