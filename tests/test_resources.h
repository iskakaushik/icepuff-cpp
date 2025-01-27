#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "icypuff/local_input_file.h"

namespace icypuff {
namespace testing {

class TestResources {
 public:
  static std::filesystem::path GetResourcePath(
      const std::string& resource_name) {
    return std::filesystem::current_path() / "tests/resources" / resource_name;
  }

  static void EnsureResourceDirectories() {
    // No need to create files, they should already exist
    std::filesystem::create_directories(GetResourcePath("v1"));
  }

  static std::unique_ptr<LocalInputFile> CreateInputFile(
      const std::string& resource_name) {
    return std::make_unique<LocalInputFile>(GetResourcePath(resource_name));
  }
};

}  // namespace testing
}  // namespace icypuff