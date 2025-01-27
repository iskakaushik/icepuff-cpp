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
  // Factory method that returns a unique_ptr to ensure ownership semantics
  static Result<std::unique_ptr<FileMetadata>> Create(FileMetadataParams&& params);

  // Constructor is public but ownership is still enforced through unique_ptr
  explicit FileMetadata(FileMetadataParams&& params);

  ICYPUFF_DISALLOW_COPY_ASSIGN_AND_MOVE(FileMetadata);

  ~FileMetadata();

  // Getters
  const std::vector<std::unique_ptr<BlobMetadata>>& blobs() const;
  const std::unordered_map<std::string, std::string>& properties() const;

 private:
  std::vector<std::unique_ptr<BlobMetadata>> blobs_;
  std::unordered_map<std::string, std::string> properties_;
};

}  // namespace icypuff