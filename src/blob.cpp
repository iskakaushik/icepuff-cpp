#include "icypuff/blob.h"

namespace icypuff {

Result<std::unique_ptr<Blob>> Blob::Create(const BlobParams& params) {
  if (params.type.empty()) {
    return {ErrorCode::kInvalidArgument, "type is empty"};
  }

  if (params.blob_data.empty()) {
    return {ErrorCode::kInvalidArgument, "blob_data is empty"};
  }

  return std::make_unique<Blob>(params);
}

Blob::Blob(const BlobParams& params)
    : type_(params.type),
      input_fields_(params.input_fields),
      snapshot_id_(params.snapshot_id),
      sequence_number_(params.sequence_number),
      blob_data_(params.blob_data),
      requested_compression_(params.requested_compression),
      properties_(params.properties) {}

Blob::~Blob() = default;

const std::string& Blob::type() const { return type_; }

const std::vector<int>& Blob::input_fields() const { return input_fields_; }

int64_t Blob::snapshot_id() const { return snapshot_id_; }

int64_t Blob::sequence_number() const { return sequence_number_; }

const std::vector<uint8_t>& Blob::blob_data() const { return blob_data_; }

CompressionCodec Blob::requested_compression() const {
  return requested_compression_;
}

const std::unordered_map<std::string, std::string>& Blob::properties() const {
  return properties_;
}

}  // namespace icypuff
