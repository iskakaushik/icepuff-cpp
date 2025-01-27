#pragma once

#include <filesystem>
#include <string>

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

 private:
  std::filesystem::path path_;
};

}  // namespace icypuff