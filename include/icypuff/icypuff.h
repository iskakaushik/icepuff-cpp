#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "icypuff/compression_codec.h"
#include "icypuff/icypuff_reader.h"
#include "icypuff/icypuff_writer.h"
#include "icypuff/output_file.h"

namespace icypuff {

// Forward declarations
class IcypuffWriter;
class IcypuffReader;

// Builder for IcypuffWriter
class IcypuffWriteBuilder {
 public:
  explicit IcypuffWriteBuilder(std::unique_ptr<OutputFile> output_file);

  // Sets file-level property to be written
  IcypuffWriteBuilder& set(const std::string& property,
                           const std::string& value);

  // Sets file-level properties to be written
  IcypuffWriteBuilder& set_all(
      const std::unordered_map<std::string, std::string>& props);

  // Sets file-level created_by property
  IcypuffWriteBuilder& created_by(const std::string& application_identifier);

  // Configures the writer to compress the footer
  IcypuffWriteBuilder& compress_footer();

  // Configures the writer to compress the blobs
  IcypuffWriteBuilder& compress_blobs(CompressionCodec compression);

  // Build and return the IcypuffWriter
  Result<std::unique_ptr<IcypuffWriter>> build();

 private:
  std::unique_ptr<OutputFile> output_file_;
  std::unordered_map<std::string, std::string> properties_;
  bool compress_footer_ = false;
  CompressionCodec default_blob_compression_ = CompressionCodec::None;
};

// Builder for IcypuffReader
class IcypuffReadBuilder {
 public:
  explicit IcypuffReadBuilder(std::unique_ptr<InputFile> input_file);

  // Passes known file size to the reader
  IcypuffReadBuilder& with_file_size(int64_t size);

  // Passes known footer size to the reader
  IcypuffReadBuilder& with_footer_size(int64_t size);

  // Build and return the IcypuffReader
  Result<std::unique_ptr<IcypuffReader>> build();

 private:
  std::unique_ptr<InputFile> input_file_;
  std::optional<int64_t> file_size_;
  std::optional<int64_t> footer_size_;
};

// Utility class for reading and writing Icypuff files
class Icypuff {
 public:
  // Prevent instantiation
  Icypuff() = delete;

  // Factory methods
  static IcypuffWriteBuilder write(std::unique_ptr<OutputFile> output_file);
  static IcypuffReadBuilder read(std::unique_ptr<InputFile> input_file);
};

}  // namespace icypuff