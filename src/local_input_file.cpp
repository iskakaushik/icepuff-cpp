#include "icypuff/local_input_file.h"

#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <fstream>
#include <system_error>

#include "icypuff/seekable_input_stream.h"

namespace icypuff {

namespace {

class LocalSeekableInputStream : public SeekableInputStream {
 public:
  explicit LocalSeekableInputStream(const std::filesystem::path& path)
      : stream_(path, std::ios::binary) {
    if (!stream_) {
      spdlog::error("Failed to open stream for file: {}", path.string());
      return;
    }
    spdlog::debug("Successfully opened stream for file: {}", path.string());
  }

  Result<size_t> read(uint8_t* buffer, size_t length) override {
    stream_.read(reinterpret_cast<char*>(buffer), length);
    if (stream_.bad()) {
      spdlog::error("Failed to read {} bytes from stream", length);
      return Result<size_t>{ErrorCode::kInvalidArgument,
                            "Failed to read from file"};
    }
    auto bytes_read = static_cast<size_t>(stream_.gcount());
    spdlog::debug("Successfully read {} bytes from stream", bytes_read);
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
    : path_(std::filesystem::absolute(path)) {
  spdlog::debug("Created LocalInputFile from string path: {}", path_.string());
}

LocalInputFile::LocalInputFile(const std::filesystem::path& path)
    : path_(std::filesystem::absolute(path)) {
  spdlog::debug("Created LocalInputFile from filesystem path: {}",
                path_.string());
}

Result<int64_t> LocalInputFile::length() const {
  std::error_code ec;
  auto size = std::filesystem::file_size(path_, ec);
  if (ec) {
    spdlog::error("Failed to get file size for {}: {}", path_.string(),
                  ec.message());
    return Result<int64_t>{ErrorCode::kInvalidArgument, ec.message()};
  }

  spdlog::debug("File size for {}: {}", path_.string(), size);
  return Result<int64_t>{static_cast<int64_t>(size)};
}

Result<std::unique_ptr<SeekableInputStream>> LocalInputFile::new_stream()
    const {
  spdlog::debug("Creating new stream for file: {}", path_.string());
  auto stream = std::make_unique<LocalSeekableInputStream>(path_);
  if (!stream->is_valid()) {
    spdlog::error("Failed to create valid stream for file: {}", path_.string());
    return Result<std::unique_ptr<SeekableInputStream>>{
        ErrorCode::kInvalidArgument, "Failed to open file"};
  }
  spdlog::debug("Successfully created stream for file: {}", path_.string());
  return Result<std::unique_ptr<SeekableInputStream>>{std::move(stream)};
}

std::string LocalInputFile::location() const { return path_.string(); }

bool LocalInputFile::exists() const {
  bool exists = std::filesystem::exists(path_);
  spdlog::debug("File {} {}", path_.string(),
                exists ? "exists" : "does not exist");
  return exists;
}

}  // namespace icypuff
