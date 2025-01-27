#pragma once

#include <optional>
#include <string_view>

namespace icypuff {

enum class CompressionCodec {
  None,  // No compression
  Lz4,   // LZ4 single compression frame with content size present 
  Zstd   // Zstandard single compression frame with content size present
};

inline std::optional<std::string_view> GetCodecName(CompressionCodec codec) {
  switch (codec) {
    case CompressionCodec::None:
      return std::nullopt;
    case CompressionCodec::Lz4:
      return "lz4";
    case CompressionCodec::Zstd:
      return "zstd";
  }
  return std::nullopt;
}

}  // namespace icypuff
