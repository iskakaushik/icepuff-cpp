#include "icypuff/file_metadata_parser.h"

#include <nlohmann/json.hpp>

namespace icypuff {

namespace {

Result<std::unique_ptr<BlobMetadata>> ParseBlobMetadata(const nlohmann::json& json) {
  BlobMetadataParams params;
  
  // Required fields
  if (!json.contains(FileMetadataParser::kType)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'type'"};
  }
  if (!json[FileMetadataParser::kType].is_string()) {
    return {ErrorCode::kInvalidArgument, "Field 'type' must be a string"};
  }
  params.type = json[FileMetadataParser::kType].get<std::string>();

  if (!json.contains(FileMetadataParser::kFields)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'fields'"};
  }
  if (!json[FileMetadataParser::kFields].is_array()) {
    return {ErrorCode::kInvalidArgument, "Field 'fields' must be an array"};
  }
  params.input_fields = json[FileMetadataParser::kFields].get<std::vector<int>>();

  if (!json.contains(FileMetadataParser::kSnapshotId)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'snapshot-id'"};
  }
  if (!json[FileMetadataParser::kSnapshotId].is_number()) {
    return {ErrorCode::kInvalidArgument, "Field 'snapshot-id' must be a number"};
  }
  params.snapshot_id = json[FileMetadataParser::kSnapshotId].get<int64_t>();

  if (!json.contains(FileMetadataParser::kSequenceNumber)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'sequence-number'"};
  }
  if (!json[FileMetadataParser::kSequenceNumber].is_number()) {
    return {ErrorCode::kInvalidArgument, "Field 'sequence-number' must be a number"};
  }
  params.sequence_number = json[FileMetadataParser::kSequenceNumber].get<int64_t>();

  if (!json.contains(FileMetadataParser::kOffset)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'offset'"};
  }
  if (!json[FileMetadataParser::kOffset].is_number()) {
    return {ErrorCode::kInvalidArgument, "Field 'offset' must be a number"};
  }
  params.offset = json[FileMetadataParser::kOffset].get<int64_t>();

  if (!json.contains(FileMetadataParser::kLength)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'length'"};
  }
  if (!json[FileMetadataParser::kLength].is_number()) {
    return {ErrorCode::kInvalidArgument, "Field 'length' must be a number"};
  }
  params.length = json[FileMetadataParser::kLength].get<int64_t>();

  // Optional fields
  if (json.contains(FileMetadataParser::kCompressionCodec)) {
    if (!json[FileMetadataParser::kCompressionCodec].is_string()) {
      return {ErrorCode::kInvalidArgument, "Field 'compression-codec' must be a string"};
    }
    params.compression_codec = json[FileMetadataParser::kCompressionCodec].get<std::string>();
  }

  if (json.contains(FileMetadataParser::kProperties)) {
    if (!json[FileMetadataParser::kProperties].is_object()) {
      return {ErrorCode::kInvalidArgument, "Field 'properties' must be an object"};
    }
    params.properties = json[FileMetadataParser::kProperties].get<std::unordered_map<std::string, std::string>>();
  }

  return BlobMetadata::Create(params);
}

nlohmann::json SerializeBlobMetadata(const BlobMetadata& metadata) {
  nlohmann::json json;

  json[FileMetadataParser::kType] = metadata.type();
  json[FileMetadataParser::kFields] = metadata.input_fields();
  json[FileMetadataParser::kSnapshotId] = metadata.snapshot_id();
  json[FileMetadataParser::kSequenceNumber] = metadata.sequence_number();
  json[FileMetadataParser::kOffset] = metadata.offset();
  json[FileMetadataParser::kLength] = metadata.length();

  if (metadata.compression_codec()) {
    json[FileMetadataParser::kCompressionCodec] = *metadata.compression_codec();
  }

  if (!metadata.properties().empty()) {
    json[FileMetadataParser::kProperties] = metadata.properties();
  }

  return json;
}

}  // namespace

Result<std::string> FileMetadataParser::ToJson(const FileMetadata& metadata, bool pretty) {
  nlohmann::json json;

  // Serialize blobs
  json[kBlobs] = nlohmann::json::array();
  for (const auto& blob : metadata.blobs()) {
    json[kBlobs].push_back(SerializeBlobMetadata(*blob));
  }

  // Serialize properties if not empty
  if (!metadata.properties().empty()) {
    json[kProperties] = metadata.properties();
  }

  // Return formatted string
  return pretty ? json.dump(2) : json.dump();
}

Result<std::unique_ptr<FileMetadata>> FileMetadataParser::FromJson(const std::string& json_str) {
  auto result = nlohmann::json::parse(json_str, nullptr, false);
  if (result.is_discarded()) {
    return {ErrorCode::kInvalidArgument, "Invalid JSON string"};
  }
  auto json = std::move(result);

  // Parse blobs
  if (!json.contains(kBlobs)) {
    return {ErrorCode::kInvalidArgument, "Missing required field 'blobs'"};
  }
  if (!json[kBlobs].is_array()) {
    return {ErrorCode::kInvalidArgument, "'blobs' must be an array"};
  }

  FileMetadataParams params;
  
  for (const auto& blob_json : json[kBlobs]) {
    auto blob_result = ParseBlobMetadata(blob_json);
    if (!blob_result.ok()) {
      return {ErrorCode::kInvalidArgument, "Failed to parse blob metadata"};
    }
    params.blobs.push_back(std::move(blob_result.value()));
  }

  // Parse properties if present
  if (json.contains(kProperties)) {
    if (!json[kProperties].is_object()) {
      return {ErrorCode::kInvalidArgument, "Field 'properties' must be an object"};
    }
    params.properties = json[kProperties].get<std::unordered_map<std::string, std::string>>();
  }

  return FileMetadata::Create(std::move(params));
}

}  // namespace icypuff 