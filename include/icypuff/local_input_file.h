#pragma once

#include <memory>
#include <filesystem>
#include <string>
#include <vector>

#include "icypuff/input_file.h"

namespace icypuff {

class LocalInputFile : public InputFile {
 public:
  explicit LocalInputFile(const std::string& path);
  explicit LocalInputFile(const std::filesystem::path& path);

  // InputFile implementation
  Result<int64_t> length() const override;
  Result<std::unique_ptr<SeekableInputStream>> new_stream() const override;
  std::string location() const override;
  bool exists() const override;

  // Helper method to read a range of bytes from the file
  Result<std::vector<uint8_t>> read_at(int64_t offset, int64_t length) const;

 private:
  std::filesystem::path path_;
};

}  // namespace icypuff