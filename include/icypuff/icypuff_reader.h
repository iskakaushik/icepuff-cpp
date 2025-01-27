#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

#include "icypuff/input_file.h"
#include "icypuff/result.h"
#include "icypuff/file_metadata.h"
#include "icypuff/blob_metadata.h"
#include "icypuff/seekable_input_stream.h"
#include "icypuff/format_constants.h"
#include "icypuff/compression_codec.h"

namespace icypuff {

// Forward declarations
class BlobMetadata;

class IcypuffReader {
 public:
  // Constructor
  IcypuffReader(std::unique_ptr<InputFile> input_file, 
                std::optional<int64_t> file_size = std::nullopt,
                std::optional<int64_t> footer_size = std::nullopt);

  // Get all blob metadata from the file
  Result<std::vector<std::unique_ptr<BlobMetadata>>> get_blobs();

  // Get file properties
  const std::unordered_map<std::string, std::string>& properties() const;

  // Read a blob's data
  Result<std::vector<uint8_t>> read_blob(const BlobMetadata& blob);

  // Close the reader
  Result<void> close();

  ICYPUFF_DISALLOW_COPY_ASSIGN_AND_MOVE(IcypuffReader);

  ~IcypuffReader() = default;

 private:
  // Helper methods
  Result<void> read_file_metadata();
  Result<int> get_footer_size();
  Result<std::vector<uint8_t>> read_input(int64_t offset, int length);
  Result<void> check_magic(const std::vector<uint8_t>& data, int offset);
  Result<std::vector<uint8_t>> decompress_data(const std::vector<uint8_t>& data, 
                                              const std::optional<std::string>& codec_name);
  Result<std::string> decompress_footer(const std::vector<uint8_t>& footer_data,
                                      int footer_struct_offset,
                                      int footer_payload_size);

  // Member variables
  std::unique_ptr<InputFile> input_file_;
  std::unique_ptr<SeekableInputStream> input_stream_;
  int64_t file_size_;
  std::optional<int> known_footer_size_;
  std::unique_ptr<FileMetadata> known_file_metadata_;
};

}  // namespace icypuff 