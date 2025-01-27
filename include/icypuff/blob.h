#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "icypuff/compression_codec.h"
#include "icypuff/macros.h"
#include "icypuff/result.h"

namespace icypuff {

struct BlobParams {
  std::string type;
  std::vector<int> input_fields;
  int64_t snapshot_id;
  int64_t sequence_number;
  std::vector<uint8_t> blob_data;
  CompressionCodec requested_compression;
  std::unordered_map<std::string, std::string> properties;
};

class Blob {
 public:
  // Full constructor
  static Result<Blob> Create(const BlobParams& params);

  // Allow move operations
  Blob(Blob&&) = default;
  Blob& operator=(Blob&&) = default;

  ICYPUFF_DISALLOW_COPY_AND_ASSIGN(Blob);

  // Getters
  const std::string& type() const;
  const std::vector<int>& input_fields() const;
  int64_t snapshot_id() const;
  int64_t sequence_number() const;
  const std::vector<uint8_t>& blob_data() const;
  CompressionCodec requested_compression() const;
  const std::unordered_map<std::string, std::string>& properties() const;

 private:
  explicit Blob(const BlobParams& params);

  std::string type_;
  std::vector<int> input_fields_;
  int64_t snapshot_id_;
  int64_t sequence_number_;
  std::vector<uint8_t> blob_data_;
  CompressionCodec requested_compression_;
  std::unordered_map<std::string, std::string> properties_;
};

}  // namespace icypuff