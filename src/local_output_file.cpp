#include "icypuff/local_output_file.h"

#include <fstream>
#include <spdlog/spdlog.h>

#include "icypuff/local_input_file.h"
#include "icypuff/position_output_stream.h"

namespace icypuff {

namespace {

class LocalPositionOutputStream : public PositionOutputStream {
 public:
  explicit LocalPositionOutputStream(const std::filesystem::path& path,
                                     bool overwrite)
      : stream_(path, std::ios::binary | std::ios::out |
                          (overwrite ? std::ios::trunc : std::ios::app)) {
    if (!stream_) {
      spdlog::error("Failed to open output stream for path: {}", path.string());
      return;  // Error will be handled by caller
    }
    spdlog::debug("Successfully opened output stream for path: {}", path.string());
  }

  Result<void> write(const uint8_t* buffer, size_t length) override {
    spdlog::debug("Writing {} bytes to output stream", length);
    stream_.write(static_cast<const char*>(static_cast<const void*>(buffer)),
                  length);
    if (stream_.bad()) {
      spdlog::error("Failed to write {} bytes to output stream", length);
      return Result<void>{ErrorCode::kInvalidArgument,
                          "Failed to write to file"};
    }
    return Result<void>{};
  }

  Result<int64_t> position() const override {
    auto pos = stream_.tellp();
    if (pos == -1) {
      spdlog::error("Failed to get position in output stream");
      return Result<int64_t>{ErrorCode::kInvalidArgument,
                             "Failed to get position in file"};
    }
    auto pos_int = static_cast<int64_t>(pos);
    spdlog::debug("Current position in output stream: {}", pos_int);
    return Result<int64_t>{pos_int};
  }

  Result<void> flush() override {
    spdlog::debug("Flushing output stream");
    stream_.flush();
    if (stream_.fail()) {
      spdlog::error("Failed to flush output stream");
      return Result<void>{ErrorCode::kInvalidArgument, "Failed to flush file"};
    }
    return Result<void>{};
  }

  Result<void> close() override {
    spdlog::debug("Closing output stream");
    stream_.close();
    if (stream_.fail()) {
      spdlog::error("Failed to close output stream");
      return Result<void>{ErrorCode::kInvalidArgument, "Failed to close file"};
    }
    return Result<void>{};
  }

  bool is_valid() const { return static_cast<bool>(stream_); }

 private:
  mutable std::ofstream stream_;  // mutable because tellp() is not const
};

}  // namespace

LocalOutputFile::LocalOutputFile(const std::string& path) : path_(path) {
  spdlog::debug("Created LocalOutputFile with path: {}", path);
}

LocalOutputFile::LocalOutputFile(const std::filesystem::path& path)
    : path_(path) {
  spdlog::debug("Created LocalOutputFile with path: {}", path.string());
}

Result<std::unique_ptr<PositionOutputStream>> LocalOutputFile::create() {
  spdlog::debug("Attempting to create new file at: {}", path_.string());
  if (std::filesystem::exists(path_)) {
    spdlog::error("File already exists at path: {}", path_.string());
    return Result<std::unique_ptr<PositionOutputStream>>{
        ErrorCode::kInvalidArgument, "File already exists"};
  }
  auto stream = std::make_unique<LocalPositionOutputStream>(path_, false);
  if (!stream->is_valid()) {
    spdlog::error("Failed to create file at path: {}", path_.string());
    return Result<std::unique_ptr<PositionOutputStream>>{
        ErrorCode::kInvalidArgument, "Failed to create file"};
  }
  spdlog::debug("Successfully created new file at: {}", path_.string());
  return Result<std::unique_ptr<PositionOutputStream>>{std::move(stream)};
}

Result<std::unique_ptr<PositionOutputStream>>
LocalOutputFile::create_or_overwrite() {
  spdlog::debug("Attempting to create or overwrite file at: {}", path_.string());
  auto stream = std::make_unique<LocalPositionOutputStream>(path_, true);
  if (!stream->is_valid()) {
    spdlog::error("Failed to create or overwrite file at path: {}", path_.string());
    return Result<std::unique_ptr<PositionOutputStream>>{
        ErrorCode::kInvalidArgument, "Failed to create file"};
  }
  spdlog::debug("Successfully created or overwrote file at: {}", path_.string());
  return Result<std::unique_ptr<PositionOutputStream>>{std::move(stream)};
}

std::string LocalOutputFile::location() const { return path_.string(); }

Result<std::unique_ptr<InputFile>> LocalOutputFile::to_input_file() const {
  spdlog::debug("Converting output file to input file: {}", path_.string());
  return Result<std::unique_ptr<InputFile>>{
      std::make_unique<LocalInputFile>(path_)};
}

}  // namespace icypuff
