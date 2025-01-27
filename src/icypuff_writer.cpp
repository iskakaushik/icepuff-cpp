#include "icypuff/icypuff_writer.h"

#include <lz4frame.h>
#include <spdlog/spdlog.h>
#include <zstd.h>

#include <memory>
#include <vector>

#include "icypuff/file_metadata.h"
#include "icypuff/file_metadata_parser.h"
#include "icypuff/format_constants.h"

namespace icypuff {

IcypuffWriter::IcypuffWriter(
    std::unique_ptr<OutputFile> output_file,
    std::unordered_map<std::string, std::string> properties,
    bool compress_footer,
    CompressionCodec default_blob_compression)
    : output_file_(std::move(output_file)),
      properties_(std::move(properties)),
      footer_compression_(compress_footer ? CompressionCodec::Zstd
                                        : CompressionCodec::None),
      default_blob_compression_(default_blob_compression) {
  auto stream_result = output_file_->create();
  if (!stream_result.ok()) {
    spdlog::error("Failed to create output stream: {}",
                  stream_result.error().message);
    return;
  }
  output_stream_ = std::move(stream_result).value();
  spdlog::debug("Successfully initialized writer");
}

Result<std::unique_ptr<BlobMetadata>> IcypuffWriter::write_blob(
    const uint8_t* data, size_t length, const std::string& type,
    const std::vector<int>& fields, int64_t snapshot_id, int64_t sequence_number,
    std::optional<CompressionCodec> compression,
    const std::unordered_map<std::string, std::string>& properties) {
  if (finished_) {
    return {ErrorCode::kInvalidState, "Writer is already finished"};
  }

  if (!output_stream_) {
    return {ErrorCode::kStreamNotInitialized, "Writer is not initialized"};
  }

  auto header_result = write_header_if_needed();
  if (!header_result.ok()) {
    return {header_result.error().code, header_result.error().message};
  }

  auto pos_result = output_stream_->position();
  if (!pos_result.ok()) {
    return {pos_result.error().code, pos_result.error().message};
  }
  int64_t offset = pos_result.value();

  CompressionCodec codec =
      compression.value_or(default_blob_compression_);
  auto compressed_data = compress_data(data, length, codec);
  if (!compressed_data.ok()) {
    return {compressed_data.error().code, compressed_data.error().message};
  }

  auto write_result = output_stream_->write(compressed_data.value().data(),
                                          compressed_data.value().size());
  if (!write_result.ok()) {
    return {write_result.error().code, write_result.error().message};
  }

  BlobMetadataParams params;
  params.type = type;
  params.input_fields = fields;
  params.snapshot_id = snapshot_id;
  params.sequence_number = sequence_number;
  params.offset = offset;
  params.length = compressed_data.value().size();
  params.compression_codec = GetCodecName(codec);
  params.properties = properties;

  auto metadata = BlobMetadata::Create(params);
  if (!metadata.ok()) {
    return {metadata.error().code, metadata.error().message};
  }

  written_blobs_metadata_.push_back(std::move(metadata).value());
  return BlobMetadata::Create(params);
}

Result<int64_t> IcypuffWriter::file_size() const {
  if (!file_size_) {
    return {ErrorCode::kInvalidArgument, "File size not available until closed"};
  }
  return file_size_.value();
}

Result<int64_t> IcypuffWriter::footer_size() const {
  if (!footer_size_) {
    return {ErrorCode::kInvalidArgument, "Footer size not available until closed"};
  }
  return footer_size_.value();
}

const std::vector<std::unique_ptr<BlobMetadata>>&
IcypuffWriter::written_blobs_metadata() const {
  return written_blobs_metadata_;
}

Result<void> IcypuffWriter::close() {
  if (finished_) {
    return Result<void>();
  }

  if (!output_stream_) {
    return {ErrorCode::kStreamNotInitialized, "Writer is not initialized"};
  }

  auto header_result = write_header_if_needed();
  if (!header_result.ok()) {
    return header_result;
  }

  auto pos_result = output_stream_->position();
  if (!pos_result.ok()) {
    return {pos_result.error().code, pos_result.error().message};
  }
  int64_t footer_offset = pos_result.value();

  auto footer_result = write_footer();
  if (!footer_result.ok()) {
    return footer_result;
  }

  pos_result = output_stream_->position();
  if (!pos_result.ok()) {
    return {pos_result.error().code, pos_result.error().message};
  }

  footer_size_ = pos_result.value() - footer_offset;
  file_size_ = pos_result.value();
  finished_ = true;

  auto close_result = output_stream_->close();
  if (!close_result.ok()) {
    return close_result;
  }

  output_stream_.reset();
  return Result<void>();
}

Result<void> IcypuffWriter::write_header_if_needed() {
  if (header_written_) {
    return Result<void>();
  }

  auto write_result = output_stream_->write(MAGIC, MAGIC_LENGTH);
  if (!write_result.ok()) {
    return write_result;
  }

  header_written_ = true;
  return Result<void>();
}

Result<void> IcypuffWriter::write_footer() {
  // Write start magic
  auto write_result = output_stream_->write(MAGIC, MAGIC_LENGTH);
  if (!write_result.ok()) {
    return write_result;
  }

  // Create file metadata
  FileMetadataParams params;
  params.blobs = written_blobs_metadata_;
  params.properties = properties_;

  auto metadata = FileMetadata::Create(std::move(params));
  if (!metadata.ok()) {
    return {metadata.error().code, metadata.error().message};
  }

  // Convert metadata to JSON and compress if needed
  auto json_result = FileMetadataParser::ToJson(*metadata.value());
  if (!json_result.ok()) {
    return {json_result.error().code, json_result.error().message};
  }

  std::string json_str = std::move(json_result).value();
  auto compressed_json = compress_data(
      reinterpret_cast<const uint8_t*>(json_str.data()),
      json_str.size(),
      footer_compression_);
  if (!compressed_json.ok()) {
    return {compressed_json.error().code, compressed_json.error().message};
  }

  // Write compressed JSON
  write_result = output_stream_->write(compressed_json.value().data(),
                                     compressed_json.value().size());
  if (!write_result.ok()) {
    return write_result;
  }

  // Write footer struct
  std::vector<uint8_t> footer_struct(FOOTER_STRUCT_LENGTH);
  write_integer_little_endian(footer_struct.data(),
                             FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET,
                             compressed_json.value().size());

  // Write flags
  uint32_t flags = 0;
  if (footer_compression_ != CompressionCodec::None) {
    flags |= (1 << static_cast<int>(FooterFlag::FOOTER_PAYLOAD_COMPRESSED));
  }
  write_integer_little_endian(footer_struct.data(), FOOTER_STRUCT_FLAGS_OFFSET,
                             flags);

  // Write footer magic
  std::memcpy(footer_struct.data() + FOOTER_STRUCT_MAGIC_OFFSET, MAGIC,
              MAGIC_LENGTH);

  write_result = output_stream_->write(footer_struct.data(), footer_struct.size());
  if (!write_result.ok()) {
    return write_result;
  }

  return Result<void>();
}

Result<std::vector<uint8_t>> IcypuffWriter::compress_data(
    const uint8_t* data, size_t length, CompressionCodec codec) {
  switch (codec) {
    case CompressionCodec::None:
      return std::vector<uint8_t>(data, data + length);

    case CompressionCodec::Lz4: {
      size_t max_dst_size = LZ4F_compressFrameBound(length, nullptr);
      std::vector<uint8_t> compressed(max_dst_size);

      LZ4F_preferences_t prefs = LZ4F_INIT_PREFERENCES;
      prefs.frameInfo.contentSize = length;

      size_t result = LZ4F_compressFrame(compressed.data(), compressed.size(),
                                        data, length, &prefs);
      if (LZ4F_isError(result)) {
        return {ErrorCode::kCompressionError,
                "Failed to compress data with LZ4"};
      }

      compressed.resize(result);
      return compressed;
    }

    case CompressionCodec::Zstd: {
      size_t max_dst_size = ZSTD_compressBound(length);
      std::vector<uint8_t> compressed(max_dst_size);

      size_t const result =
          ZSTD_compress(compressed.data(), compressed.size(), data, length, 1);
      if (ZSTD_isError(result)) {
        return {ErrorCode::kCompressionError,
                "Failed to compress data with Zstd"};
      }

      compressed.resize(result);
      return compressed;
    }
  }

  return {ErrorCode::kInternalError, "Unknown compression codec"};
}

}  // namespace icypuff 
