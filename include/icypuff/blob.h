#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "icypuff/macros.h"
#include "icypuff/compression_codec.h"
#include "icypuff/result.h"

namespace icypuff {

class Blob {
public:
  // Constructor with default compression and empty properties
  static Result<Blob> Create(
      std::string type,
      std::vector<int> input_fields,
      int64_t snapshot_id,
      int64_t sequence_number,
      std::vector<uint8_t> blob_data) noexcept;

  // Full constructor
  static Result<Blob> Create(
      std::string type,
      std::vector<int> input_fields,
      int64_t snapshot_id,
      int64_t sequence_number,
      std::vector<uint8_t> blob_data,
      std::optional<CompressionCodec> requested_compression,
      std::unordered_map<std::string, std::string> properties) noexcept;

  // Allow move operations
  Blob(Blob&&) noexcept = default;
  Blob& operator=(Blob&&) noexcept = default;

  ICYPUFF_DISALLOW_COPY_AND_ASSIGN(Blob);

  // Getters
  const std::string &type() const noexcept;
  const std::vector<int> &input_fields() const noexcept;
  int64_t snapshot_id() const noexcept;
  int64_t sequence_number() const noexcept;
  const std::vector<uint8_t> &blob_data() const noexcept;
  const std::optional<CompressionCodec> &requested_compression() const noexcept;
  const std::unordered_map<std::string, std::string> &properties() const noexcept;

private:
  Blob(std::string type,
       std::vector<int> input_fields,
       int64_t snapshot_id,
       int64_t sequence_number,
       std::vector<uint8_t> blob_data,
       std::optional<CompressionCodec> requested_compression,
       std::unordered_map<std::string, std::string> properties) noexcept;

  std::string type_;
  std::vector<int> input_fields_;
  int64_t snapshot_id_;
  int64_t sequence_number_;
  std::vector<uint8_t> blob_data_;
  std::optional<CompressionCodec> requested_compression_;
  std::unordered_map<std::string, std::string> properties_;
};

} // namespace icypuff