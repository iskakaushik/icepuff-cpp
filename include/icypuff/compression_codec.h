#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "icypuff/result.h"

namespace icypuff {

class CompressionCodec {
public:
  enum Value {
    NONE,   // No compression
    LZ4,    // LZ4 single compression frame with content size present
    ZSTD,   // Zstandard single compression frame with content size present
  };

  // Construct from enum value
  constexpr CompressionCodec(Value value) : value_(value) {}

  // Get the string name of the codec
  std::optional<std::string_view> codec_name() const noexcept {
    switch (value_) {
      case NONE: return std::nullopt;
      case LZ4: return "lz4";
      case ZSTD: return "zstd";
    }
    return std::nullopt;  // Unreachable but silences warning
  }

  // Get enum value
  constexpr Value value() const noexcept { return value_; }

  // Construct from string name
  static Result<CompressionCodec> from_name(std::optional<std::string_view> name) noexcept {
    if (!name) {
      return CompressionCodec(NONE);
    }
    
    if (*name == "lz4") return CompressionCodec(LZ4);
    if (*name == "zstd") return CompressionCodec(ZSTD);
    
    return Result<CompressionCodec>(
        ErrorCode::kUnknownCodec,
        "Unknown codec name");
  }

  // Comparison operators
  constexpr bool operator==(const CompressionCodec& other) const noexcept {
    return value_ == other.value_;
  }
  constexpr bool operator!=(const CompressionCodec& other) const noexcept {
    return value_ != other.value_;
  }

private:
  Value value_;
};

} // namespace icypuff 
