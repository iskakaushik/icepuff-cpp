#include "icypuff/blob.h"

#include <stdexcept>

namespace icypuff {

Result<Blob> Blob::Create(std::string type, std::vector<int> input_fields,
                          int64_t snapshot_id, int64_t sequence_number,
                          std::vector<uint8_t> blob_data) {
  return Create(std::move(type), std::move(input_fields), snapshot_id,
                sequence_number, std::move(blob_data), std::nullopt,
                std::unordered_map<std::string, std::string>{});
}

Result<Blob> Blob::Create(
    std::string type, std::vector<int> input_fields, int64_t snapshot_id,
    int64_t sequence_number, std::vector<uint8_t> blob_data,
    std::optional<CompressionCodec> requested_compression,
    std::unordered_map<std::string, std::string> properties) {
  if (type.empty()) {
    return Result<Blob>(ErrorCode::kInvalidArgument, "type is empty");
  }
  if (blob_data.empty()) {
    return Result<Blob>(ErrorCode::kInvalidArgument, "blob_data is empty");
  }

  return Blob(std::move(type), std::move(input_fields), snapshot_id,
              sequence_number, std::move(blob_data), requested_compression,
              std::move(properties));
}

Blob::Blob(std::string type, std::vector<int> input_fields, int64_t snapshot_id,
           int64_t sequence_number, std::vector<uint8_t> blob_data,
           std::optional<CompressionCodec> requested_compression,
           std::unordered_map<std::string, std::string> properties)
    : type_(std::move(type)),
      input_fields_(std::move(input_fields)),
      snapshot_id_(snapshot_id),
      sequence_number_(sequence_number),
      blob_data_(std::move(blob_data)),
      requested_compression_(requested_compression),
      properties_(std::move(properties)) {}

const std::string& Blob::type() const { return type_; }

const std::vector<int>& Blob::input_fields() const { return input_fields_; }

int64_t Blob::snapshot_id() const { return snapshot_id_; }

int64_t Blob::sequence_number() const { return sequence_number_; }

const std::vector<uint8_t>& Blob::blob_data() const { return blob_data_; }

const std::optional<CompressionCodec>& Blob::requested_compression() const {
  return requested_compression_;
}

const std::unordered_map<std::string, std::string>& Blob::properties() const {
  return properties_;
}

}  // namespace icypuff
