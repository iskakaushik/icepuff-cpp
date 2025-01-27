#include "icypuff/icypuff_writer.h"

#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>
#include <random>

#include "icypuff/icypuff.h"
#include "test_resources.h"

namespace icypuff {
namespace {

using ::icypuff::testing::TestResources;

// Test constants
constexpr int EMPTY_PUFFIN_UNCOMPRESSED_FOOTER_SIZE =
    28;  // 4 (magic) + 4 (payload size) + 4 (flags) + 4 (magic) + 12 (payload)

// Helper function to generate a random 8 character ID
std::string generate_uuid() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 15);
  static const char* digits = "0123456789abcdef";

  std::string id;
  id.reserve(8);

  for (int i = 0; i < 8; i++) {
    id += digits[dis(gen)];
  }

  return id;
}

class IcypuffWriterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize logging
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

    TestResources::EnsureResourceDirectories();
  }
};

TEST_F(IcypuffWriterTest, EmptyFooterUncompressed) {
  std::string filename = generate_uuid() + "-empty-puffin-uncompressed.bin";
  auto output_file = TestResources::CreateOutputFile(filename);
  auto writer_result = Icypuff::write(std::move(output_file)).build();
  ASSERT_TRUE(writer_result.ok()) << writer_result.error().message;
  auto writer = std::move(writer_result).value();

  // Test footer size before closing
  auto footer_size_result = writer->footer_size();
  ASSERT_FALSE(footer_size_result.ok());
  EXPECT_EQ(footer_size_result.error().code, ErrorCode::kInvalidArgument);
  EXPECT_EQ(footer_size_result.error().message, "Footer size not available until closed");

  // Close writer
  auto close_result = writer->close();
  ASSERT_TRUE(close_result.ok()) << close_result.error().message;

  // Test footer size after closing
  footer_size_result = writer->footer_size();
  ASSERT_TRUE(footer_size_result.ok()) << footer_size_result.error().message;
  EXPECT_EQ(footer_size_result.value(), EMPTY_PUFFIN_UNCOMPRESSED_FOOTER_SIZE);

  // Compare with reference file
  auto reference_file = TestResources::CreateInputFile("v1/empty-puffin-uncompressed.bin");
  auto reference_data = reference_file->read_at(0, reference_file->length().value());
  ASSERT_TRUE(reference_data.ok()) << reference_data.error().message;

  auto input_file = TestResources::CreateInputFile(filename);
  auto output_data = input_file->read_at(0, input_file->length().value());
  ASSERT_TRUE(output_data.ok()) << output_data.error().message;

  EXPECT_EQ(output_data.value(), reference_data.value());
  EXPECT_TRUE(writer->written_blobs_metadata().empty());
}

TEST_F(IcypuffWriterTest, WriteMetricDataUncompressed) {
  std::string filename = generate_uuid() + "-sample-metric-data-uncompressed.bin";
  auto output_file = TestResources::CreateOutputFile(filename);
  auto writer_result = Icypuff::write(std::move(output_file))
                          .created_by("Test 1234")
                          .build();
  ASSERT_TRUE(writer_result.ok()) << writer_result.error().message;
  auto writer = std::move(writer_result).value();

  // Write first blob
  std::string data1 = "abcdefghi";
  auto blob1_result = writer->write_blob(
      reinterpret_cast<const uint8_t*>(data1.data()),
      data1.size(),
      "some-blob",
      std::vector<int>{1},
      2,  // snapshot_id
      1   // sequence_number
  );
  ASSERT_TRUE(blob1_result.ok()) << blob1_result.error().message;

  // Write second blob
  // Create binary data with null character and emoji
  std::string binary_str = "some blob \0 binary data 🤯 that is not very very very very very very long, is it?";
  auto blob2_result = writer->write_blob(
      reinterpret_cast<const uint8_t*>(binary_str.data()),
      binary_str.size(),
      "some-other-blob",
      std::vector<int>{2},
      2,  // snapshot_id
      1   // sequence_number
  );
  ASSERT_TRUE(blob2_result.ok()) << blob2_result.error().message;

  // Verify written blobs metadata
  const auto& blobs = writer->written_blobs_metadata();
  ASSERT_EQ(blobs.size(), 2);

  // Check first blob metadata
  const auto& first_blob = blobs[0];
  EXPECT_EQ(first_blob->type(), "some-blob");
  EXPECT_EQ(first_blob->input_fields(), std::vector<int>{1});
  EXPECT_TRUE(first_blob->properties().empty());

  // Check second blob metadata
  const auto& second_blob = blobs[1];
  EXPECT_EQ(second_blob->type(), "some-other-blob");
  EXPECT_EQ(second_blob->input_fields(), std::vector<int>{2});
  EXPECT_TRUE(second_blob->properties().empty());

  // Close writer
  auto close_result = writer->close();
  ASSERT_TRUE(close_result.ok()) << close_result.error().message;

  // Compare with reference file
  auto reference_file = TestResources::CreateInputFile("v1/sample-metric-data-uncompressed.bin");
  auto reference_data = reference_file->read_at(0, reference_file->length().value());
  ASSERT_TRUE(reference_data.ok()) << reference_data.error().message;

  auto input_file = TestResources::CreateInputFile(filename);
  auto output_data = input_file->read_at(0, input_file->length().value());
  ASSERT_TRUE(output_data.ok()) << output_data.error().message;

  EXPECT_EQ(output_data.value(), reference_data.value());
}

TEST_F(IcypuffWriterTest, DISABLED_WriteMetricDataCompressedZstd) {
  std::string filename = generate_uuid() + "-sample-metric-data-compressed-zstd.bin";
  auto output_file = TestResources::CreateOutputFile(filename);
  auto writer_result = Icypuff::write(std::move(output_file))
                          .created_by("Test 1234")
                          .compress_blobs(CompressionCodec::Zstd)
                          .build();
  ASSERT_TRUE(writer_result.ok()) << writer_result.error().message;
  auto writer = std::move(writer_result).value();

  // Write first blob
  std::string data1 = "abcdefghi";
  auto blob1_result = writer->write_blob(
      reinterpret_cast<const uint8_t*>(data1.data()),
      data1.size(),
      "some-blob",
      std::vector<int>{1},
      2,  // snapshot_id
      1   // sequence_number
  );
  ASSERT_TRUE(blob1_result.ok()) << blob1_result.error().message;

  // Write second blob
  std::string binary_str = "some blob \0 binary data 🤯 that is not very very very very very very long, is it?";
  std::string_view binary_view(binary_str.data(), binary_str.size());
  auto blob2_result = writer->write_blob(
      reinterpret_cast<const uint8_t*>(binary_view.data() + 4),
      binary_view.size() - 8,  // -8 to exclude the xxx prefix and suffix
      "some-other-blob",
      std::vector<int>{2},
      2,  // snapshot_id
      1   // sequence_number
  );
  ASSERT_TRUE(blob2_result.ok()) << blob2_result.error().message;

  // Verify written blobs metadata
  const auto& blobs = writer->written_blobs_metadata();
  ASSERT_EQ(blobs.size(), 2);

  // Check first blob metadata
  const auto& first_blob = blobs[0];
  EXPECT_EQ(first_blob->type(), "some-blob");
  EXPECT_EQ(first_blob->input_fields(), std::vector<int>{1});
  EXPECT_TRUE(first_blob->properties().empty());
  EXPECT_EQ(first_blob->compression_codec(), "zstd");

  // Check second blob metadata
  const auto& second_blob = blobs[1];
  EXPECT_EQ(second_blob->type(), "some-other-blob");
  EXPECT_EQ(second_blob->input_fields(), std::vector<int>{2});
  EXPECT_TRUE(second_blob->properties().empty());
  EXPECT_EQ(second_blob->compression_codec(), "zstd");

  // Close writer
  auto close_result = writer->close();
  ASSERT_TRUE(close_result.ok()) << close_result.error().message;

  // Compare with reference file
  auto reference_file = TestResources::CreateInputFile("v1/sample-metric-data-compressed-zstd.bin");
  auto reference_data = reference_file->read_at(0, reference_file->length().value());
  ASSERT_TRUE(reference_data.ok()) << reference_data.error().message;

  auto input_file = TestResources::CreateInputFile(filename);
  auto output_data = input_file->read_at(0, input_file->length().value());
  ASSERT_TRUE(output_data.ok()) << output_data.error().message;

  EXPECT_EQ(output_data.value(), reference_data.value());
}

}  // namespace
}  // namespace icypuff 