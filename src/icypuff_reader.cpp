#include "icypuff/icypuff_reader.h"

#include <vector>
#include <memory>
#include <lz4frame.h>
#include <zstd.h>

#include "icypuff/file_metadata_parser.h"

namespace icypuff {

IcypuffReader::IcypuffReader(std::unique_ptr<InputFile> input_file,
                           std::optional<int64_t> file_size,
                           std::optional<int64_t> footer_size)
    : input_file_(std::move(input_file)) {
  
  auto length_result = input_file_->length();
  if (!length_result.ok()) {
    file_size_ = 0;
    return;
  }
  
  file_size_ = file_size.value_or(length_result.value());
  
  if (footer_size.has_value()) {
    int64_t size = footer_size.value();
    if (size <= 0 || size > file_size_ - MAGIC_LENGTH) {
      return;
    }
    known_footer_size_ = static_cast<int>(size);
  }
  
  auto stream_result = input_file_->new_stream();
  if (!stream_result.ok()) {
    return;
  }
  input_stream_ = std::move(stream_result).value();
}

Result<std::vector<std::unique_ptr<BlobMetadata>>> IcypuffReader::get_blobs() {
  auto metadata_result = read_file_metadata();
  if (!metadata_result.ok()) {
    return Result<std::vector<std::unique_ptr<BlobMetadata>>>(
        metadata_result.error().code, metadata_result.error().message);
  }
  
  std::vector<std::unique_ptr<BlobMetadata>> blobs;
  for (const auto& blob : known_file_metadata_->blobs()) {
    BlobMetadataParams params;
    params.type = blob->type();
    params.input_fields = blob->input_fields();
    params.snapshot_id = blob->snapshot_id();
    params.sequence_number = blob->sequence_number();
    params.offset = blob->offset();
    params.length = blob->length();
    params.compression_codec = blob->compression_codec();
    params.properties = blob->properties();
    
    auto new_blob = BlobMetadata::Create(params);
    if (!new_blob.ok()) {
      return Result<std::vector<std::unique_ptr<BlobMetadata>>>(
          new_blob.error().code, new_blob.error().message);
    }
    blobs.push_back(std::move(new_blob).value());
  }
  return blobs;
}

const std::unordered_map<std::string, std::string>& IcypuffReader::properties() const {
  static const std::unordered_map<std::string, std::string> empty_map;
  return known_file_metadata_ ? known_file_metadata_->properties() : empty_map;
}

Result<std::vector<uint8_t>> IcypuffReader::read_blob(const BlobMetadata& blob) {
  if (!input_stream_) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kInvalidArgument, "Reader is not initialized");
  }
  
  auto seek_result = input_stream_->seek(blob.offset());
  if (!seek_result.ok()) {
    return Result<std::vector<uint8_t>>(
        seek_result.error().code, seek_result.error().message);
  }
  
  std::vector<uint8_t> data(blob.length());
  auto read_result = input_stream_->read(data.data(), data.size());
  if (!read_result.ok()) {
    return Result<std::vector<uint8_t>>(
        read_result.error().code, read_result.error().message);
  }
  
  if (read_result.value() != data.size()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kInvalidArgument, "Failed to read complete blob data");
  }
  
  return decompress_data(data, blob.compression_codec());
}

Result<std::vector<uint8_t>> IcypuffReader::decompress_data(
    const std::vector<uint8_t>& data,
    const std::optional<std::string>& codec_name) {
  
  auto codec = GetCodecFromName(codec_name);
  if (!codec.has_value()) {
    return {ErrorCode::kUnknownCodec, "Unknown compression codec: " + codec_name.value_or("none")};
  }

  switch (codec.value()) {
    case CompressionCodec::None:
      return data;

    case CompressionCodec::Lz4: {
      // Get decompressed size from LZ4 frame
      LZ4F_decompressionContext_t ctx;
      auto err = LZ4F_createDecompressionContext(&ctx, LZ4F_VERSION);
      if (LZ4F_isError(err)) {
        return {ErrorCode::kInvalidArgument, "Failed to create LZ4 decompression context"};
      }

      size_t src_size = data.size();
      size_t dest_size = 0;
      err = LZ4F_getFrameInfo(ctx, nullptr, data.data(), &src_size);
      if (LZ4F_isError(err)) {
        LZ4F_freeDecompressionContext(ctx);
        return {ErrorCode::kInvalidArgument, "Failed to get LZ4 frame info"};
      }

      // Allocate output buffer (LZ4 frame header contains the decompressed size)
      std::vector<uint8_t> decompressed(err);  // err contains the decompressed size
      size_t decompressed_size = decompressed.size();
      
      err = LZ4F_decompress(ctx, decompressed.data(), &decompressed_size,
                           data.data(), &src_size, nullptr);
      LZ4F_freeDecompressionContext(ctx);
      
      if (LZ4F_isError(err)) {
        return {ErrorCode::kInvalidArgument, "Failed to decompress LZ4 data"};
      }
      
      decompressed.resize(decompressed_size);
      return decompressed;
    }

    case CompressionCodec::Zstd: {
      // Get decompressed size from Zstd frame
      unsigned long long const decompressed_size = ZSTD_getFrameContentSize(data.data(), data.size());
      if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        return {ErrorCode::kInvalidArgument, "Invalid Zstd data"};
      }
      if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        return {ErrorCode::kInvalidArgument, "Unknown Zstd content size"};
      }

      // Allocate output buffer
      std::vector<uint8_t> decompressed(decompressed_size);
      
      size_t const err = ZSTD_decompress(decompressed.data(), decompressed.size(),
                                       data.data(), data.size());
      if (ZSTD_isError(err)) {
        return {ErrorCode::kInvalidArgument, "Failed to decompress Zstd data"};
      }
      
      decompressed.resize(err);
      return decompressed;
    }
  }

  // Should never reach here since we handle all enum values
  return {ErrorCode::kUnknownCodec, "Unknown compression codec"};
}

Result<void> IcypuffReader::close() {
  if (input_stream_) {
    auto result = input_stream_->close();
    input_stream_.reset();
    return result;
  }
  return Result<void>();
}

Result<void> IcypuffReader::read_file_metadata() {
  if (known_file_metadata_) {
    return Result<void>();
  }
  
  auto footer_size_result = get_footer_size();
  if (!footer_size_result.ok()) {
    return Result<void>(footer_size_result.error().code, 
                       footer_size_result.error().message);
  }
  int footer_size = footer_size_result.value();
  
  auto footer_data = read_input(file_size_ - footer_size, footer_size);
  if (!footer_data.ok()) {
    return Result<void>(footer_data.error().code, footer_data.error().message);
  }
  
  auto magic_check = check_magic(footer_data.value(), 0);  // Footer start magic
  if (!magic_check.ok()) {
    return magic_check;
  }
  
  int footer_struct_offset = footer_size - FOOTER_STRUCT_LENGTH;
  magic_check = check_magic(footer_data.value(), footer_struct_offset + FOOTER_STRUCT_MAGIC_OFFSET);
  if (!magic_check.ok()) {
    return magic_check;
  }
  
  int footer_payload_size = *reinterpret_cast<const int*>(
      &footer_data.value()[footer_struct_offset + FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET]);
  
  if (footer_size != FOOTER_START_MAGIC_LENGTH + footer_payload_size + FOOTER_STRUCT_LENGTH) {
    return Result<void>(ErrorCode::kInvalidArgument, "Invalid footer payload size");
  }

  auto json_result = decompress_footer(footer_data.value(), footer_struct_offset, footer_payload_size);
  if (!json_result.ok()) {
    return Result<void>(json_result.error().code, json_result.error().message);
  }
  
  auto metadata_result = FileMetadataParser::FromJson(json_result.value());
  if (!metadata_result.ok()) {
    return Result<void>(metadata_result.error().code, metadata_result.error().message);
  }
  
  known_file_metadata_ = std::move(metadata_result).value();
  return Result<void>();
}

Result<std::string> IcypuffReader::decompress_footer(
    const std::vector<uint8_t>& footer_data,
    int footer_struct_offset,
    int footer_payload_size) {
  // Read compression flags
  uint32_t flags = *reinterpret_cast<const uint32_t*>(
      &footer_data[footer_struct_offset + FOOTER_STRUCT_FLAGS_OFFSET]);
  
  // Extract the footer data
  std::vector<uint8_t> footer_payload(
      footer_data.begin() + MAGIC_LENGTH,
      footer_data.begin() + MAGIC_LENGTH + footer_payload_size);

  // Bit 0 indicates LZ4 compression
  // Bit 1 indicates Zstd compression
  std::optional<std::string> compression_codec;
  if (flags & 0x1) {
    compression_codec = "lz4";
  } else if (flags & 0x2) {
    compression_codec = "zstd";
  }

  // Decompress the footer data if needed
  if (compression_codec.has_value()) {
    auto decompressed = decompress_data(footer_payload, compression_codec);
    if (!decompressed.ok()) {
      std::string error_msg = "Failed to decompress footer: ";
      error_msg += decompressed.error().message;
      return Result<std::string>(decompressed.error().code, error_msg);
    }
    return std::string(reinterpret_cast<const char*>(decompressed.value().data()),
                      decompressed.value().size());
  }
  
  return std::string(reinterpret_cast<const char*>(footer_payload.data()),
                    footer_payload.size());
}

Result<int> IcypuffReader::get_footer_size() {
  if (known_footer_size_) {
    return *known_footer_size_;
  }
  
  if (file_size_ < FOOTER_STRUCT_LENGTH) {
    return Result<int>(ErrorCode::kInvalidArgument, "File too small to contain footer");
  }
  
  auto footer_struct = read_input(file_size_ - FOOTER_STRUCT_LENGTH, FOOTER_STRUCT_LENGTH);
  if (!footer_struct.ok()) {
    return Result<int>(footer_struct.error().code, footer_struct.error().message);
  }
  
  auto magic_check = check_magic(footer_struct.value(), FOOTER_STRUCT_MAGIC_OFFSET);
  if (!magic_check.ok()) {
    return Result<int>(magic_check.error().code, magic_check.error().message);
  }
  
  int footer_payload_size = *reinterpret_cast<const int*>(
      &footer_struct.value()[FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET]);
  
  known_footer_size_ = FOOTER_START_MAGIC_LENGTH + footer_payload_size + FOOTER_STRUCT_LENGTH;
  return *known_footer_size_;
}

Result<std::vector<uint8_t>> IcypuffReader::read_input(int64_t offset, int length) {
  if (!input_stream_) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kInvalidArgument, "Reader is not initialized");
  }
  
  auto seek_result = input_stream_->seek(offset);
  if (!seek_result.ok()) {
    return Result<std::vector<uint8_t>>(
        seek_result.error().code, seek_result.error().message);
  }
  
  std::vector<uint8_t> data(length);
  auto read_result = input_stream_->read(data.data(), length);
  if (!read_result.ok()) {
    return Result<std::vector<uint8_t>>(
        read_result.error().code, read_result.error().message);
  }
  
  if (read_result.value() != static_cast<size_t>(length)) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kInvalidArgument, "Failed to read complete data");
  }
  
  return data;
}

Result<void> IcypuffReader::check_magic(const std::vector<uint8_t>& data, int offset) {
  if (offset + MAGIC_LENGTH > static_cast<int>(data.size())) {
    return Result<void>(ErrorCode::kInvalidArgument, "Data too small to contain magic");
  }
  
  for (int i = 0; i < MAGIC_LENGTH; i++) {
    if (data[offset + i] != MAGIC[i]) {
      return Result<void>(ErrorCode::kInvalidArgument, "Invalid magic bytes");
    }
  }
  
  return Result<void>();
}

}  // namespace icypuff 