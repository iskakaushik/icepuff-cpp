#include "icypuff/file_metadata.h"

namespace icypuff {

Result<FileMetadata> FileMetadata::Create(const FileMetadataParams& params) {
  if (params.blobs.empty()) {
    return {ErrorCode::kInvalidArgument, "blobs is empty"};
  }
  return FileMetadata(params);
}

FileMetadata::FileMetadata(const FileMetadataParams& params)
    : blobs_(std::move(params.blobs)),
      properties_(std::move(params.properties)) {}

const std::vector<BlobMetadata>& FileMetadata::blobs() const { return blobs_; }

const std::unordered_map<std::string, std::string>& FileMetadata::properties()
    const {
  return properties_;
}

}  // namespace icypuff 