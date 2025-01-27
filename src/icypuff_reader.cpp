#include "icypuff/icypuff_reader.h"

#include <vector>
#include <memory>

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
  
  // TODO: Handle decompression based on blob.compression_codec()
  return data;
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
  
  // TODO: Handle footer compression based on flags
  
  int footer_payload_size = *reinterpret_cast<const int*>(
      &footer_data.value()[footer_struct_offset + FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET]);
  
  if (footer_size != FOOTER_START_MAGIC_LENGTH + footer_payload_size + FOOTER_STRUCT_LENGTH) {
    return Result<void>(ErrorCode::kInvalidArgument, "Invalid footer payload size");
  }
  
  std::string json_str(reinterpret_cast<const char*>(&footer_data.value()[MAGIC_LENGTH]), 
                      footer_payload_size);
  
  auto metadata_result = FileMetadataParser::FromJson(json_str);
  if (!metadata_result.ok()) {
    return Result<void>(metadata_result.error().code, metadata_result.error().message);
  }
  
  known_file_metadata_ = std::move(metadata_result).value();
  return Result<void>();
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