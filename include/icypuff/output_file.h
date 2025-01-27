#pragma once

#include <memory>
#include <string>

#include "icypuff/input_file.h"
#include "icypuff/result.h"
#include "icypuff/position_output_stream.h"
namespace icypuff {

class OutputFile {
 public:
  virtual ~OutputFile() = default;

  // Create a new file and return a PositionOutputStream to it.
  // If the file already exists, this will return an error.
  virtual Result<std::unique_ptr<PositionOutputStream>> create() = 0;

  // Create a new file and return a PositionOutputStream to it.
  // If the file already exists, it will be overwritten.
  virtual Result<std::unique_ptr<PositionOutputStream>> create_or_overwrite() = 0;

  // Return the location this output file will create
  virtual std::string location() const = 0;

  // Return an InputFile for the location of this output file
  virtual Result<std::unique_ptr<InputFile>> to_input_file() const = 0;
};

}  // namespace icypuff 