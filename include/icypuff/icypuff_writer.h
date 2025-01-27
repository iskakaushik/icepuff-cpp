#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "icypuff/compression_codec.h"
#include "icypuff/output_file.h"

namespace icypuff {

class IcypuffWriter {
 public:
  virtual ~IcypuffWriter() = default;

  // Write a blob to the file
  virtual Result<void> write_blob(const uint8_t* data, size_t length,
                                  const std::string& type,
                                  const std::vector<int>& fields,
                                  int64_t snapshot_id,
                                  int64_t sequence_number) = 0;

  // Close the file and write the footer
  virtual Result<void> close() = 0;
};

}  // namespace icypuff