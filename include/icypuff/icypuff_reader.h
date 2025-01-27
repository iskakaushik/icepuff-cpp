#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "icypuff/input_file.h"
#include "icypuff/result.h"

namespace icypuff {

// Forward declarations
class BlobMetadata;

class IcypuffReader {
 public:
  virtual ~IcypuffReader() = default;

  // Get all blob metadata from the file
  virtual Result<std::vector<std::unique_ptr<BlobMetadata>>> get_blobs() = 0;

  // Get file properties
  virtual const std::unordered_map<std::string, std::string>& properties() const = 0;

  // Read a blob's data
  virtual Result<std::vector<uint8_t>> read_blob(const BlobMetadata& blob) = 0;

  // Close the reader
  virtual Result<void> close() = 0;
};

}  // namespace icypuff 