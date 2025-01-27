#pragma once

#include <memory>
#include <filesystem>
#include <string>

#include "icypuff/output_file.h"

namespace icypuff {

class LocalOutputFile : public OutputFile {
 public:
  explicit LocalOutputFile(const std::string& path);
  explicit LocalOutputFile(const std::filesystem::path& path);

  // OutputFile implementation
  Result<std::unique_ptr<PositionOutputStream>> create() override;
  Result<std::unique_ptr<PositionOutputStream>> create_or_overwrite() override;
  std::string location() const override;
  Result<std::unique_ptr<InputFile>> to_input_file() const override;

 private:
  std::filesystem::path path_;
};

}  // namespace icypuff