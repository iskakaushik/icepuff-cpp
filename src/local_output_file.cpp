#include "icypuff/local_output_file.h"

#include <fstream>

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
      return;  // Error will be handled by caller
    }
  }

  Result<void> write(const uint8_t* buffer, size_t length) override {
    stream_.write(reinterpret_cast<const char*>(buffer), length);
    if (stream_.bad()) {
      return Result<void>{ErrorCode::kInvalidArgument,
                          "Failed to write to file"};
    }
    return Result<void>{};
  }

  Result<int64_t> position() const override {
    auto pos = stream_.tellp();
    if (pos == -1) {
      return Result<int64_t>{ErrorCode::kInvalidArgument,
                             "Failed to get position in file"};
    }
    return Result<int64_t>{static_cast<int64_t>(pos)};
  }

  Result<void> flush() override {
    stream_.flush();
    if (stream_.fail()) {
      return Result<void>{ErrorCode::kInvalidArgument, "Failed to flush file"};
    }
    return Result<void>{};
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
  mutable std::ofstream stream_;  // mutable because tellp() is not const
};

}  // namespace

LocalOutputFile::LocalOutputFile(const std::string& path) : path_(path) {}

LocalOutputFile::LocalOutputFile(const std::filesystem::path& path)
    : path_(path) {}

Result<std::unique_ptr<PositionOutputStream>> LocalOutputFile::create() {
  if (std::filesystem::exists(path_)) {
    return Result<std::unique_ptr<PositionOutputStream>>{
        ErrorCode::kInvalidArgument, "File already exists"};
  }
  auto stream = std::make_unique<LocalPositionOutputStream>(path_, false);
  if (!stream->is_valid()) {
    return Result<std::unique_ptr<PositionOutputStream>>{
        ErrorCode::kInvalidArgument, "Failed to create file"};
  }
  return Result<std::unique_ptr<PositionOutputStream>>{std::move(stream)};
}

Result<std::unique_ptr<PositionOutputStream>>
LocalOutputFile::create_or_overwrite() {
  auto stream = std::make_unique<LocalPositionOutputStream>(path_, true);
  if (!stream->is_valid()) {
    return Result<std::unique_ptr<PositionOutputStream>>{
        ErrorCode::kInvalidArgument, "Failed to create file"};
  }
  return Result<std::unique_ptr<PositionOutputStream>>{std::move(stream)};
}

std::string LocalOutputFile::location() const { return path_.string(); }

Result<std::unique_ptr<InputFile>> LocalOutputFile::to_input_file() const {
  return Result<std::unique_ptr<InputFile>>{
      std::make_unique<LocalInputFile>(path_)};
}

}  // namespace icypuff
