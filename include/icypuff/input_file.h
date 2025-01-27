#pragma once

#include <memory>
#include <string>

#include "icypuff/result.h"
#include "icypuff/seekable_input_stream.h"

namespace icypuff {

class InputFile {
 public:
  virtual ~InputFile() = default;

  // Returns the total length of the file, in bytes
  virtual Result<int64_t> length() const = 0;

  // Opens a new SeekableInputStream for the underlying data file
  virtual Result<std::unique_ptr<SeekableInputStream>> new_stream() const = 0;

  // The fully-qualified location of the input file as a string
  virtual std::string location() const = 0;

  // Checks whether the file exists
  virtual bool exists() const = 0;
};

}  // namespace icypuff