#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "icypuff/macros.h"
#include "icypuff/result.h"

namespace icypuff {

struct BlobMetadataParams {
  std::string type;
  std::vector<int> input_fields;
  int64_t snapshot_id;
  int64_t sequence_number;
  int64_t offset;
  int64_t length;
  std::optional<std::string> compression_codec;
  std::unordered_map<std::string, std::string> properties;
};

class BlobMetadata {
 public:
  static Result<BlobMetadata> Create(const BlobMetadataParams& params);

  // Allow move operations
  BlobMetadata(BlobMetadata&&) = default;
  BlobMetadata& operator=(BlobMetadata&&) = default;

  // Getters
  const std::string& type() const;
  const std::vector<int>& input_fields() const;
  int64_t snapshot_id() const;
  int64_t sequence_number() const;
  int64_t offset() const;
  int64_t length() const;
  const std::optional<std::string>& compression_codec() const;
  const std::unordered_map<std::string, std::string>& properties() const;

 private:
  explicit BlobMetadata(const BlobMetadataParams& params);

  ICYPUFF_DISALLOW_COPY_AND_ASSIGN(BlobMetadata);

  std::string type_;
  std::vector<int> input_fields_;
  int64_t snapshot_id_;
  int64_t sequence_number_;
  int64_t offset_;
  int64_t length_;
  std::optional<std::string> compression_codec_;
  std::unordered_map<std::string, std::string> properties_;
};

}  // namespace icypuff