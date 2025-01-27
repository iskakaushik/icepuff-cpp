#pragma once

#include <cstdint>

namespace icypuff {

// Magic bytes and lengths
static constexpr uint8_t MAGIC[] = {0x50, 0x46, 0x46, 0x4E};  // "PFFN"
static constexpr int MAGIC_LENGTH = 4;

// Footer structure
static constexpr int FOOTER_STRUCT_LENGTH = 16;
static constexpr int FOOTER_STRUCT_FLAGS_LENGTH = 4;
static constexpr int FOOTER_STRUCT_FLAGS_OFFSET = 4;
static constexpr int FOOTER_STRUCT_PAYLOAD_SIZE_OFFSET = 8;
static constexpr int FOOTER_STRUCT_MAGIC_OFFSET = 12;
static constexpr int FOOTER_START_MAGIC_LENGTH = 4;

}  // namespace icypuff 