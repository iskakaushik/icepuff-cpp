#pragma once

#include <memory>
#include <string>

#include "icypuff/file_metadata.h"
#include "icypuff/result.h"

namespace icypuff {

class FileMetadataParser {
 public:
  // Prevent instantiation
  FileMetadataParser() = delete;

  // Convert FileMetadata to JSON string
  static Result<std::string> ToJson(const FileMetadata& metadata, bool pretty = false);

  // Parse FileMetadata from JSON string
  static Result<std::unique_ptr<FileMetadata>> FromJson(const std::string& json);

  // JSON field names
  static constexpr const char* kBlobs = "blobs";
  static constexpr const char* kProperties = "properties";
  static constexpr const char* kType = "type";
  static constexpr const char* kFields = "fields";
  static constexpr const char* kSnapshotId = "snapshot-id";
  static constexpr const char* kSequenceNumber = "sequence-number";
  static constexpr const char* kOffset = "offset";
  static constexpr const char* kLength = "length";
  static constexpr const char* kCompressionCodec = "compression-codec";
};

}  // namespace icypuff 
