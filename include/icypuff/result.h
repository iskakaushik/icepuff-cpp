#pragma once

#include <string_view>
#include <variant>

namespace icypuff {

enum class ErrorCode {
  kOk = 0,
  kInvalidArgument,
  
  // File format errors
  kInvalidMagic,           // Magic bytes don't match
  kInvalidFooterSize,      // Footer size is invalid
  kInvalidFooterPayload,   // Footer payload is malformed
  kInvalidFileLength,      // File is too short
  
  // Stream errors
  kStreamNotInitialized,   // Reader/Writer stream not initialized
  kStreamSeekError,        // Failed to seek in stream
  kStreamReadError,        // Failed to read from stream
  kStreamWriteError,       // Failed to write to stream
  kIncompleteRead,         // Read fewer bytes than requested
  kIncompleteWrite,        // Wrote fewer bytes than requested
  
  // Compression errors
  kUnknownCodec,           // Unknown compression codec
  kCompressionError,       // Error during compression
  kDecompressionError,     // Error during decompression
  
  // Other errors
  kUnimplemented,          // Feature not implemented
  kInternalError,          // Unexpected internal error
};

struct ResultError {
  ErrorCode code;
  std::string_view message;
};

template <typename T>
class Result {
 public:
  using Error = ResultError;

  // Construct with value
  constexpr Result(T value) : var_(std::move(value)) {}

  // Construct with error
  constexpr Result(ErrorCode code, std::string_view message)
      : var_(Error{code, message}) {}

  // Check if result contains a value
  constexpr bool ok() const { return std::holds_alternative<T>(var_); }

  // Get the contained value. Must check ok() first.
  constexpr const T& value() const& { return std::get<T>(var_); }

  constexpr T&& value() && { return std::get<T>(std::move(var_)); }

  // Get the error. Must check !ok() first.
  constexpr const Error& error() const& { return std::get<Error>(var_); }

 private:
  std::variant<T, Error> var_;
};

// Specialization for void - simpler implementation without std::monostate
template <>
class Result<void> {
 public:
  using Error = ResultError;

  // Construct success
  constexpr Result() : success_(true) {}

  // Construct with error
  constexpr Result(ErrorCode code, std::string_view message)
      : success_(false), error_{code, message} {}

  // Check if result contains a value
  constexpr bool ok() const { return success_; }

  // Get the error. Must check !ok() first.
  constexpr const Error& error() const& { return error_; }

 private:
  bool success_;
  Error error_{ErrorCode::kOk, ""};  // Default error state, only valid when !success_
};

}  // namespace icypuff