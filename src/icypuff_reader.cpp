#include "icypuff/icypuff_reader.h"

#include <vector>
#include <memory>
#include <lz4frame.h>
#include <zstd.h>
#include <spdlog/spdlog.h>

#include "icypuff/file_metadata_parser.h"
#include "icypuff/format_constants.h"

namespace icypuff {

IcypuffReader::IcypuffReader(std::unique_ptr<InputFile> input_file,
                           std::optional<int64_t> file_size,
                           std::optional<int64_t> footer_size)
    : input_file_(std::move(input_file)) {
  
  auto length_result = input_file_->length();
  if (!length_result.ok()) {
    spdlog::error("Failed to get file length: {}", length_result.error().message);
    file_size_ = 0;
    return;
  }
  
  file_size_ = file_size.value_or(length_result.value());
  spdlog::debug("File size: {}", file_size_);
  
  if (footer_size.has_value()) {
    int64_t size = footer_size.value();
    spdlog::debug("Using provided footer size: {}", size);
    if (size <= FOOTER_START_MAGIC_LENGTH + FOOTER_STRUCT_LENGTH) {
      spdlog::error("Invalid footer size: {}", size);
      error_code_ = ErrorCode::kInvalidFooterSize;
      error_message_ = ERROR_INVALID_FOOTER_SIZE;
      return;
    }
    if (size > file_size_) {
      spdlog::error("Footer size {} larger than file size {}", size, file_size_);
      error_code_ = ErrorCode::kInvalidFileLength;
      error_message_ = "Footer size larger than file size";
      return;
    }
    known_footer_size_ = static_cast<int>(size);
  }
  
  auto stream_result = input_file_->new_stream();
  if (!stream_result.ok()) {
    spdlog::error("Failed to create input stream: {}", stream_result.error().message);
    error_code_ = ErrorCode::kStreamNotInitialized;
    error_message_ = ERROR_READER_NOT_INITIALIZED;
    return;
  }
  input_stream_ = std::move(stream_result).value();
  spdlog::debug("Successfully initialized reader");
}

Result<std::vector<std::unique_ptr<BlobMetadata>>> IcypuffReader::get_blobs() {
  if (error_code_ != ErrorCode::kOk) {
    return Result<std::vector<std::unique_ptr<BlobMetadata>>>(error_code_, error_message_);
  }
  
  if (!input_stream_) {
    return Result<std::vector<std::unique_ptr<BlobMetadata>>>(
        ErrorCode::kStreamNotInitialized, ERROR_READER_NOT_INITIALIZED);
  }
  
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
      spdlog::error("Failed to create blob metadata: {}", new_blob.error().message);
      return Result<std::vector<std::unique_ptr<BlobMetadata>>>(
          new_blob.error().code, new_blob.error().message);
    }
    blobs.push_back(std::move(new_blob).value());
  }
  spdlog::debug("Successfully read {} blobs", blobs.size());
  return blobs;
}

const std::unordered_map<std::string, std::string>& IcypuffReader::properties() const {
  static const std::unordered_map<std::string, std::string> empty_map;
  return known_file_metadata_ ? known_file_metadata_->properties() : empty_map;
}

Result<std::vector<uint8_t>> IcypuffReader::read_blob(const BlobMetadata& blob) {
  if (!input_stream_) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kStreamNotInitialized, ERROR_READER_NOT_INITIALIZED);
  }
  
  auto seek_result = input_stream_->seek(blob.offset());
  if (!seek_result.ok()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kStreamSeekError, seek_result.error().message);
  }
  
  std::vector<uint8_t> data(blob.length());
  auto read_result = input_stream_->read(data.data(), data.size());
  if (!read_result.ok()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kStreamReadError, read_result.error().message);
  }
  
  if (read_result.value() != data.size()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kIncompleteRead, ERROR_INCOMPLETE_BLOB_READ);
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
        return {ErrorCode::kDecompressionError, "Failed to create LZ4 decompression context"};
      }

      size_t src_size = data.size();
      size_t dest_size = 0;
      err = LZ4F_getFrameInfo(ctx, nullptr, data.data(), &src_size);
      if (LZ4F_isError(err)) {
        LZ4F_freeDecompressionContext(ctx);
        return {ErrorCode::kDecompressionError, "Failed to get LZ4 frame info"};
      }

      std::vector<uint8_t> decompressed(err);
      size_t decompressed_size = decompressed.size();
      
      err = LZ4F_decompress(ctx, decompressed.data(), &decompressed_size,
                           data.data(), &src_size, nullptr);
      LZ4F_freeDecompressionContext(ctx);
      
      if (LZ4F_isError(err)) {
        return {ErrorCode::kDecompressionError, "Failed to decompress LZ4 data"};
      }
      
      decompressed.resize(decompressed_size);
      return decompressed;
    }

    case CompressionCodec::Zstd: {
      unsigned long long const decompressed_size = ZSTD_getFrameContentSize(data.data(), data.size());
      if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
        return {ErrorCode::kDecompressionError, "Invalid Zstd data"};
      }
      if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        return {ErrorCode::kDecompressionError, "Unknown Zstd content size"};
      }

      std::vector<uint8_t> decompressed(decompressed_size);
      
      size_t const err = ZSTD_decompress(decompressed.data(), decompressed.size(),
                                       data.data(), data.size());
      if (ZSTD_isError(err)) {
        return {ErrorCode::kDecompressionError, "Failed to decompress Zstd data"};
      }
      
      decompressed.resize(err);
      return decompressed;
    }
  }

  return {ErrorCode::kInternalError, "Unknown compression codec"};
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
    spdlog::debug("Using cached file metadata");
    return Result<void>();
  }
  
  auto footer_size_result = get_footer_size();
  if (!footer_size_result.ok()) {
    return Result<void>(footer_size_result.error().code, 
                       footer_size_result.error().message);
  }
  int footer_size = footer_size_result.value();
  spdlog::debug("Footer size: {}", footer_size);
  
  auto footer_data = read_input(file_size_ - footer_size, footer_size);
  if (!footer_data.ok()) {
    return Result<void>(ErrorCode::kInvalidFooterSize, ERROR_INVALID_FOOTER_SIZE);
  }
  spdlog::debug("Successfully read {} bytes of footer data", footer_data.value().size());
  
  auto magic_check = check_magic(footer_data.value(), FOOTER_START_MAGIC_OFFSET);
  if (!magic_check.ok()) {
    return Result<void>(ErrorCode::kInvalidMagic, ERROR_INVALID_MAGIC);
  }
  
  int footer_struct_offset = footer_size - FOOTER_STRUCT_LENGTH;
  magic_check = check_magic(footer_data.value(), footer_struct_offset + FOOTER_STRUCT_MAGIC_OFFSET);
  if (!magic_check.ok()) {
    return Result<void>(ErrorCode::kInvalidMagic, ERROR_INVALID_MAGIC);
  }
  
  int footer_payload_size = read_integer_little_endian(
      footer_data.value().data() + footer_struct_offset, FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET);
  spdlog::debug("Footer payload size: {}", footer_payload_size);
  
  if (footer_size != FOOTER_START_MAGIC_LENGTH + footer_payload_size + FOOTER_STRUCT_LENGTH) {
    return Result<void>(ErrorCode::kInvalidFooterSize, ERROR_INVALID_FOOTER_SIZE);
  }

  // Extract the footer payload (JSON data) between the start magic and footer struct
  std::string json_data(
      reinterpret_cast<const char*>(footer_data.value().data() + FOOTER_START_MAGIC_LENGTH),
      footer_payload_size);
  spdlog::debug("Footer JSON: {}", json_data);
  
  auto metadata_result = FileMetadataParser::FromJson(json_data);
  if (!metadata_result.ok()) {
    return Result<void>(ErrorCode::kInvalidFooterPayload, metadata_result.error().message);
  }
  
  known_file_metadata_ = std::move(metadata_result).value();
  spdlog::debug("Successfully parsed file metadata");
  return Result<void>();
}

Result<int> IcypuffReader::get_footer_size() {
  if (known_footer_size_) {
    return *known_footer_size_;
  }
  
  if (file_size_ < FOOTER_STRUCT_LENGTH) {
    return Result<int>(ErrorCode::kInvalidFileLength, 
                      "Invalid file: file length " + std::to_string(file_size_) + 
                      " is less than minimal length of the footer tail " + 
                      std::to_string(FOOTER_STRUCT_LENGTH));
  }
  
  auto footer_struct = read_input(file_size_ - FOOTER_STRUCT_LENGTH, FOOTER_STRUCT_LENGTH);
  if (!footer_struct.ok()) {
    return Result<int>(ErrorCode::kInvalidFooterSize, ERROR_INVALID_FOOTER_SIZE);
  }
  
  auto magic_check = check_magic(footer_struct.value(), FOOTER_STRUCT_MAGIC_OFFSET);
  if (!magic_check.ok()) {
    return Result<int>(ErrorCode::kInvalidMagic, ERROR_INVALID_MAGIC);
  }
  
  int footer_payload_size = read_integer_little_endian(footer_struct.value().data(), 
                                                     FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET);
  
  int total_footer_size = FOOTER_START_MAGIC_LENGTH + footer_payload_size + FOOTER_STRUCT_LENGTH;
  if (total_footer_size <= FOOTER_START_MAGIC_LENGTH + FOOTER_STRUCT_LENGTH || 
      total_footer_size > file_size_) {
    return Result<int>(ErrorCode::kInvalidFooterSize, ERROR_INVALID_FOOTER_SIZE);
  }
  
  // Verify start magic
  auto start_magic = read_input(file_size_ - total_footer_size, FOOTER_START_MAGIC_LENGTH);
  if (!start_magic.ok()) {
    return Result<int>(ErrorCode::kInvalidFooterSize, ERROR_INVALID_FOOTER_SIZE);
  }
  
  auto start_magic_check = check_magic(start_magic.value(), FOOTER_START_MAGIC_OFFSET);
  if (!start_magic_check.ok()) {
    return Result<int>(ErrorCode::kInvalidMagic, ERROR_INVALID_MAGIC);
  }
  
  known_footer_size_ = total_footer_size;
  return total_footer_size;
}

Result<std::vector<uint8_t>> IcypuffReader::read_input(int64_t offset, int length) {
  if (!input_stream_) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kStreamNotInitialized, ERROR_READER_NOT_INITIALIZED);
  }
  
  auto seek_result = input_stream_->seek(offset);
  if (!seek_result.ok()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kStreamSeekError, seek_result.error().message);
  }
  
  std::vector<uint8_t> data(length);
  auto read_result = input_stream_->read(data.data(), data.size());
  if (!read_result.ok()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kStreamReadError, read_result.error().message);
  }
  
  if (read_result.value() != data.size()) {
    return Result<std::vector<uint8_t>>(
        ErrorCode::kIncompleteRead, ERROR_INCOMPLETE_BLOB_READ);
  }
  
  spdlog::debug("Successfully read {} bytes at offset {}", length, offset);
  return data;
}

Result<void> IcypuffReader::check_magic(const std::vector<uint8_t>& data, int offset) {
  if (offset + MAGIC_LENGTH > data.size()) {
    return Result<void>(ErrorCode::kInvalidFileLength, 
                       "Not enough data to check magic: need " + std::to_string(MAGIC_LENGTH) + 
                       " bytes, have " + std::to_string(data.size() - offset) + " bytes");
  }
  
  for (int i = 0; i < MAGIC_LENGTH; i++) {
    if (data[offset + i] != MAGIC[i]) {
      std::stringstream error_msg;
      error_msg << "Invalid file: expected magic at offset " << offset << ": ";
      error_msg << "expected " << std::hex << static_cast<int>(MAGIC[i]) 
                << ", got " << static_cast<int>(data[offset + i]) << " at position " << i;
      return Result<void>(ErrorCode::kInvalidMagic, error_msg.str());
    }
  }
  
  return Result<void>();
}

}  // namespace icypuff 