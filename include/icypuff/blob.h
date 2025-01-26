#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "icypuff/macros.h"

namespace icypuff {

enum class CompressionCodec {
  NONE,
  // Add other compression types as needed
};

class Blob {
public:
  // Constructor with default compression and empty properties
  Blob(std::string type, std::vector<int> input_fields, int64_t snapshot_id,
       int64_t sequence_number, std::vector<uint8_t> blob_data);

  // Full constructor
  Blob(std::string type, std::vector<int> input_fields, int64_t snapshot_id,
       int64_t sequence_number, std::vector<uint8_t> blob_data,
       std::optional<CompressionCodec> requested_compression,
       std::unordered_map<std::string, std::string> properties);

  // Allow move operations
  Blob(Blob&&) noexcept = default;
  Blob& operator=(Blob&&) noexcept = default;

  // Getters
  const std::string &type() const noexcept;
  const std::vector<int> &input_fields() const noexcept;
  int64_t snapshot_id() const noexcept;
  int64_t sequence_number() const noexcept;
  const std::vector<uint8_t> &blob_data() const noexcept;
  const std::optional<CompressionCodec> &requested_compression() const noexcept;
  const std::unordered_map<std::string, std::string> &
  properties() const noexcept;

private:
  ICYPUFF_DISALLOW_COPY_AND_ASSIGN(Blob);


  std::string type_;
  std::vector<int> input_fields_;
  int64_t snapshot_id_;
  int64_t sequence_number_;
  std::vector<uint8_t> blob_data_;
  std::optional<CompressionCodec> requested_compression_;
  std::unordered_map<std::string, std::string> properties_;
};

} // namespace icypuff