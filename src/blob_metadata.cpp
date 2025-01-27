#include "icypuff/blob_metadata.h"

namespace icypuff {

Result<BlobMetadata> BlobMetadata::Create(const BlobMetadataParams& params) {
  if (params.type.empty()) {
    return {ErrorCode::kInvalidArgument, "type is empty"};
  }
  if (params.input_fields.empty()) {
    return {ErrorCode::kInvalidArgument, "input_fields is empty"};
  }
  if (params.offset < 0) {
    return {ErrorCode::kInvalidArgument, "offset must be non-negative"};
  }
  if (params.length <= 0) {
    return {ErrorCode::kInvalidArgument, "length must be positive"};
  }

  return BlobMetadata(params);
}

BlobMetadata::BlobMetadata(const BlobMetadataParams& params)
    : type_(std::move(params.type)),
      input_fields_(std::move(params.input_fields)),
      snapshot_id_(params.snapshot_id),
      sequence_number_(params.sequence_number),
      offset_(params.offset),
      length_(params.length),
      compression_codec_(std::move(params.compression_codec)),
      properties_(std::move(params.properties)) {}

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