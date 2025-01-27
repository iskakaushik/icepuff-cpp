#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "icypuff/blob_metadata.h"
#include "icypuff/compression_codec.h"
#include "icypuff/output_file.h"
#include "icypuff/position_output_stream.h"
#include "icypuff/result.h"

namespace icypuff {

class IcypuffWriter {
 public:
  // Constructor
  IcypuffWriter(std::unique_ptr<OutputFile> output_file,
                std::unordered_map<std::string, std::string> properties,
                bool compress_footer,
                CompressionCodec default_blob_compression);

  virtual ~IcypuffWriter() = default;

  // Write a blob to the file
  Result<std::unique_ptr<BlobMetadata>> write_blob(
      const uint8_t* data, size_t length, const std::string& type,
      const std::vector<int>& fields, int64_t snapshot_id = 0,
      int64_t sequence_number = 0,
      std::optional<CompressionCodec> compression = std::nullopt,
      const std::unordered_map<std::string, std::string>& properties = {});

  // Get the current file size
  Result<int64_t> file_size() const;

  // Get the footer size (only valid after close)
  Result<int64_t> footer_size() const;

  // Get the list of written blobs metadata
  const std::vector<std::unique_ptr<BlobMetadata>>& written_blobs_metadata()
      const;

  // Close the file and write the footer
  Result<void> close();

  ICYPUFF_DISALLOW_COPY_ASSIGN_AND_MOVE(IcypuffWriter);

 private:
  // Helper methods
  Result<void> write_header_if_needed();
  Result<void> write_footer();
  Result<void> write_flags();
  Result<std::vector<uint8_t>> compress_data(const uint8_t* data, size_t length,
                                             CompressionCodec codec);

  // Member variables
  std::unique_ptr<OutputFile> output_file_;
  std::unique_ptr<PositionOutputStream> output_stream_;
  std::unordered_map<std::string, std::string> properties_;
  CompressionCodec footer_compression_;
  CompressionCodec default_blob_compression_;
  std::vector<std::unique_ptr<BlobMetadata>> written_blobs_metadata_;
  bool header_written_ = false;
  bool finished_ = false;
  std::optional<int64_t> footer_size_;
  std::optional<int64_t> file_size_;
};

}  // namespace icypuff