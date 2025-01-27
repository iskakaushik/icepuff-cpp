#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "icypuff/blob_metadata.h"
#include "icypuff/macros.h"
#include "icypuff/result.h"

namespace icypuff {

struct FileMetadataParams {
  std::vector<BlobMetadata> blobs;
  std::unordered_map<std::string, std::string> properties;
};

class FileMetadata {
 public:
  static Result<FileMetadata> Create(const FileMetadataParams& params);

  // Allow move operations
  FileMetadata(FileMetadata&&) = default;
  FileMetadata& operator=(FileMetadata&&) = default;

  ICYPUFF_DISALLOW_COPY_AND_ASSIGN(FileMetadata);

  // Getters
  const std::vector<BlobMetadata>& blobs() const;
  const std::unordered_map<std::string, std::string>& properties() const;

 private:
  explicit FileMetadata(const FileMetadataParams& params);

  std::vector<BlobMetadata> blobs_;
  std::unordered_map<std::string, std::string> properties_;
};

}  // namespace icypuff 