#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "icypuff/result.h"

namespace icypuff {

class PositionOutputStream {
 public:
  virtual ~PositionOutputStream() = default;

  // Write length bytes from the buffer
  virtual Result<void> write(const uint8_t* buffer, size_t length) = 0;

  // Get the current position in the stream
  virtual Result<int64_t> position() const = 0;

  // Flush any buffered data
  virtual Result<void> flush() = 0;

  // Close the stream
  virtual Result<void> close() = 0;
};

}  // namespace icypuff