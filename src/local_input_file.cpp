#include "icypuff/local_input_file.h"

#include <sys/stat.h>

#include <fstream>

#include "icypuff/format_constants.h"
#include "icypuff/seekable_input_stream.h"

namespace icypuff {

namespace {

class LocalSeekableInputStream : public SeekableInputStream {
 public:
  explicit LocalSeekableInputStream(const std::filesystem::path& path)
      : stream_(path, std::ios::binary) {}

  Result<size_t> read(uint8_t* buffer, size_t length) override {
    stream_.read(reinterpret_cast<char*>(buffer), length);
    if (stream_.bad()) {
      return Result<size_t>{ErrorCode::kInvalidArgument,
                            "Failed to read from file"};
    }
    auto bytes_read = static_cast<size_t>(stream_.gcount());
    return Result<size_t>{bytes_read};
  }

  Result<void> skip(int64_t length) override {
    stream_.seekg(length, std::ios::cur);
    if (stream_.fail()) {
      return Result<void>{ErrorCode::kInvalidArgument,
                          "Failed to skip in file"};
    }
    return Result<void>{};
  }

  Result<void> seek(int64_t position) override {
    stream_.seekg(position);
    if (stream_.fail()) {
      return Result<void>{ErrorCode::kInvalidArgument,
                          "Failed to seek in file"};
    }
    return Result<void>{};
  }

  Result<int64_t> position() const override {
    auto pos = stream_.tellg();
    if (pos == -1) {
      return Result<int64_t>{ErrorCode::kInvalidArgument,
                             "Failed to get position in file"};
    }
    return Result<int64_t>{static_cast<int64_t>(pos)};
  }

  Result<void> close() override {
    stream_.close();
    if (stream_.fail()) {
      return Result<void>{ErrorCode::kInvalidArgument, "Failed to close file"};
    }
    return Result<void>{};
  }

  bool is_valid() const { return static_cast<bool>(stream_); }

 private:
  mutable std::ifstream stream_;
};

}  // namespace

LocalInputFile::LocalInputFile(const std::string& path)
    : path_(std::filesystem::absolute(path)) {}

LocalInputFile::LocalInputFile(const std::filesystem::path& path)
    : path_(std::filesystem::absolute(path)) {}

Result<int64_t> LocalInputFile::length() const {
  std::error_code ec;
  auto size = std::filesystem::file_size(path_, ec);
  if (ec) {
    return Result<int64_t>{ErrorCode::kInvalidArgument, ec.message()};
  }
  return Result<int64_t>{static_cast<int64_t>(size)};
}

Result<std::unique_ptr<SeekableInputStream>> LocalInputFile::new_stream()
    const {
  auto stream = std::make_unique<LocalSeekableInputStream>(path_);
  if (!stream->is_valid()) {
    return Result<std::unique_ptr<SeekableInputStream>>{
        ErrorCode::kInvalidArgument, "Failed to open file"};
  }
  return Result<std::unique_ptr<SeekableInputStream>>{std::move(stream)};
}

std::string LocalInputFile::location() const { return path_.string(); }

bool LocalInputFile::exists() const { return std::filesystem::exists(path_); }

Result<std::vector<uint8_t>> LocalInputFile::read_at(int64_t offset,
                                                     int64_t length) const {
  auto stream_result = new_stream();
  if (!stream_result.ok()) {
    return Result<std::vector<uint8_t>>{stream_result.error().code,
                                        stream_result.error().message};
  }

  auto stream = std::move(stream_result).value();
  auto seek_result = stream->seek(offset);
  if (!seek_result.ok()) {
    return Result<std::vector<uint8_t>>{seek_result.error().code,
                                        seek_result.error().message};
  }

  std::vector<uint8_t> buffer(length);
  auto read_result = stream->read(buffer.data(), buffer.size());
  if (!read_result.ok()) {
    return Result<std::vector<uint8_t>>{read_result.error().code,
                                        read_result.error().message};
  }

  if (read_result.value() != length) {
    return Result<std::vector<uint8_t>>{ErrorCode::kIncompleteRead,
                                        ERROR_INCOMPLETE_BLOB_READ};
  }

  return buffer;
}

}  // namespace icypuff
