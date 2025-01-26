#include "icypuff/blob_metadata.h"

namespace icypuff {

Result<BlobMetadata> BlobMetadata::Create(
    std::string type, std::vector<int> input_fields, int64_t snapshot_id,
    int64_t sequence_number, int64_t offset, int64_t length,
    std::optional<std::string> compression_codec,
    std::unordered_map<std::string, std::string> properties) {
  if (type.empty()) {
    return Result<BlobMetadata>(ErrorCode::kInvalidArgument, "type is empty");
  }
  if (input_fields.empty()) {
    return Result<BlobMetadata>(ErrorCode::kInvalidArgument,
                                "input_fields is empty");
  }
  if (offset < 0) {
    return Result<BlobMetadata>(ErrorCode::kInvalidArgument,
                                "offset must be non-negative");
  }
  if (length <= 0) {
    return Result<BlobMetadata>(ErrorCode::kInvalidArgument,
                                "length must be positive");
  }

  return BlobMetadata(std::move(type), std::move(input_fields), snapshot_id,
                      sequence_number, offset, length,
                      std::move(compression_codec), std::move(properties));
}

BlobMetadata::BlobMetadata(
    std::string type, std::vector<int> input_fields, int64_t snapshot_id,
    int64_t sequence_number, int64_t offset, int64_t length,
    std::optional<std::string> compression_codec,
    std::unordered_map<std::string, std::string> properties)
    : type_(std::move(type)),
      input_fields_(std::move(input_fields)),
      snapshot_id_(snapshot_id),
      sequence_number_(sequence_number),
      offset_(offset),
      length_(length),
      compression_codec_(std::move(compression_codec)),
      properties_(std::move(properties)) {}

const std::string& BlobMetadata::type() const { return type_; }

const std::vector<int>& BlobMetadata::input_fields() const {
  return input_fields_;
}

int64_t BlobMetadata::snapshot_id() const { return snapshot_id_; }

int64_t BlobMetadata::sequence_number() const { return sequence_number_; }

int64_t BlobMetadata::offset() const { return offset_; }

int64_t BlobMetadata::length() const { return length_; }

const std::optional<std::string>& BlobMetadata::compression_codec() const {
  return compression_codec_;
}

const std::unordered_map<std::string, std::string>& BlobMetadata::properties()
    const {
  return properties_;
}

}  // namespace icypuff