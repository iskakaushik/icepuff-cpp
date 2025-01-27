#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "icypuff/blob_metadata.h"
#include "icypuff/macros.h"
#include "icypuff/result.h"

namespace icypuff {

struct FileMetadataParams {
  std::vector<std::unique_ptr<BlobMetadata>> blobs;
  std::unordered_map<std::string, std::string> properties;
};

class FileMetadata {
 public:
  static Result<FileMetadata> Create(FileMetadataParams&& params);

  // Allow move operations
  FileMetadata(FileMetadata&&) = default;
  FileMetadata& operator=(FileMetadata&&) = default;

  ~FileMetadata();

  // Getters
  const std::vector<std::unique_ptr<BlobMetadata>>& blobs() const;
  const std::unordered_map<std::string, std::string>& properties() const;

 private:
  ICYPUFF_DISALLOW_COPY_AND_ASSIGN(FileMetadata);

  explicit FileMetadata(FileMetadataParams&& params);

  std::vector<std::unique_ptr<BlobMetadata>> blobs_;
  std::unordered_map<std::string, std::string> properties_;
};

}  // namespace icypuff