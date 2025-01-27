#include "icypuff/icypuff.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "icypuff/compression_codec.h"
#include "icypuff/result.h"

namespace icypuff {

// IcypuffWriteBuilder implementation
IcypuffWriteBuilder::IcypuffWriteBuilder(std::unique_ptr<OutputFile> output_file)
    : output_file_(std::move(output_file)) {}

IcypuffWriteBuilder& IcypuffWriteBuilder::set(const std::string& property,
                                           const std::string& value) {
  properties_[property] = value;
  return *this;
}

IcypuffWriteBuilder& IcypuffWriteBuilder::set_all(
    const std::unordered_map<std::string, std::string>& props) {
  properties_.insert(props.begin(), props.end());
  return *this;
}

IcypuffWriteBuilder& IcypuffWriteBuilder::created_by(
    const std::string& application_identifier) {
  properties_["created_by"] = application_identifier;
  return *this;
}

IcypuffWriteBuilder& IcypuffWriteBuilder::compress_footer() {
  compress_footer_ = true;
  return *this;
}

IcypuffWriteBuilder& IcypuffWriteBuilder::compress_blobs(
    CompressionCodec compression) {
  default_blob_compression_ = compression;
  return *this;
}

Result<std::unique_ptr<IcypuffWriter>> IcypuffWriteBuilder::build() {
  // TODO: Implement IcypuffWriter creation
  return {ErrorCode::kUnimplemented, "Not implemented yet"};
}

// IcypuffReadBuilder implementation
IcypuffReadBuilder::IcypuffReadBuilder(std::unique_ptr<InputFile> input_file)
    : input_file_(std::move(input_file)) {}

IcypuffReadBuilder& IcypuffReadBuilder::with_file_size(int64_t size) {
  file_size_ = size;
  return *this;
}

IcypuffReadBuilder& IcypuffReadBuilder::with_footer_size(int64_t size) {
  footer_size_ = size;
  return *this;
}

Result<std::unique_ptr<IcypuffReader>> IcypuffReadBuilder::build() {
  // TODO: Implement IcypuffReader creation
  return {ErrorCode::kUnimplemented, "Not implemented yet"};
}

// Static factory methods
IcypuffWriteBuilder Icypuff::write(std::unique_ptr<OutputFile> output_file) {
  return IcypuffWriteBuilder(std::move(output_file));
}

IcypuffReadBuilder Icypuff::read(std::unique_ptr<InputFile> input_file) {
  return IcypuffReadBuilder(std::move(input_file));
}

}  // namespace icypuff