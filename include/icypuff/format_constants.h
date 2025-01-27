#pragma once

#include <cstdint>
#include <string_view>

namespace icypuff {

// Magic bytes and lengths
static constexpr uint8_t MAGIC[] = {0x50, 0x46, 0x41, 0x31};  // "PFA1"
static constexpr int MAGIC_LENGTH = 4;

// Error messages
static constexpr std::string_view ERROR_INVALID_MAGIC = "Invalid file: expected magic at offset";
static constexpr std::string_view ERROR_INVALID_FOOTER_SIZE = "Invalid footer size";
static constexpr std::string_view ERROR_READER_NOT_INITIALIZED = "Reader is not initialized";
static constexpr std::string_view ERROR_INCOMPLETE_BLOB_READ = "Failed to read complete blob data";

// Footer structure offsets and lengths
static constexpr int FOOTER_START_MAGIC_OFFSET = 0;
static constexpr int FOOTER_START_MAGIC_LENGTH = MAGIC_LENGTH;

static constexpr int FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET = 0;
static constexpr int FOOTER_STRUCT_FLAGS_OFFSET = FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET + 4;
static constexpr int FOOTER_STRUCT_FLAGS_LENGTH = 4;
static constexpr int FOOTER_STRUCT_MAGIC_OFFSET = FOOTER_STRUCT_FLAGS_OFFSET + FOOTER_STRUCT_FLAGS_LENGTH;
static constexpr int FOOTER_STRUCT_LENGTH = FOOTER_STRUCT_MAGIC_OFFSET + MAGIC_LENGTH;

// Footer flags
enum class FooterFlag {
  FOOTER_PAYLOAD_COMPRESSED = 0  // byte 0, bit 0
};

// Helper functions for reading/writing integers in little endian
inline uint32_t read_integer_little_endian(const uint8_t* data, int offset) {
  return static_cast<uint32_t>(data[offset]) |
         (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) |
         (static_cast<uint32_t>(data[offset + 3]) << 24);
}

inline void write_integer_little_endian(uint8_t* data, int offset, uint32_t value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
  data[offset + 2] = (value >> 16) & 0xFF;
  data[offset + 3] = (value >> 24) & 0xFF;
}

}  // namespace icypuff 