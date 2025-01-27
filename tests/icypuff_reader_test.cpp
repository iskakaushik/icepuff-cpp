#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

#include "icypuff/icypuff_reader.h"
#include "test_resources.h"

namespace icypuff {
namespace {

using ::icypuff::testing::TestResources;

// Test constants
static constexpr int EMPTY_PUFFIN_UNCOMPRESSED_FOOTER_SIZE = 24;  // 4 (magic) + 0 (payload) + 16 (footer struct) + 4 (magic)
static constexpr int SAMPLE_METRIC_DATA_COMPRESSED_ZSTD_FOOTER_SIZE = 200;  // Example size, needs to be updated

class IcypuffReaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    TestResources::EnsureResourceDirectories();
  }
};

TEST_F(IcypuffReaderTest, EmptyFooterUncompressed) {
  auto input_file = TestResources::CreateInputFile("v1/empty-puffin-uncompressed.bin");
  auto length_result = input_file->length();
  ASSERT_TRUE(length_result.ok()) << length_result.error().message;
  
  auto reader = IcypuffReader(std::move(input_file), length_result.value(), 
                            EMPTY_PUFFIN_UNCOMPRESSED_FOOTER_SIZE);
  
  auto blobs_result = reader.get_blobs();
  ASSERT_TRUE(blobs_result.ok()) << blobs_result.error().message;
  EXPECT_TRUE(blobs_result.value().empty());
  EXPECT_TRUE(reader.properties().empty());
}

TEST_F(IcypuffReaderTest, EmptyWithUnknownFooterSize) {
  auto input_file = TestResources::CreateInputFile("v1/empty-puffin-uncompressed.bin");
  auto length_result = input_file->length();
  ASSERT_TRUE(length_result.ok()) << length_result.error().message;
  
  auto reader = IcypuffReader(std::move(input_file), length_result.value());
  
  auto blobs_result = reader.get_blobs();
  ASSERT_TRUE(blobs_result.ok()) << blobs_result.error().message;
  EXPECT_TRUE(blobs_result.value().empty());
  EXPECT_TRUE(reader.properties().empty());
}

TEST_F(IcypuffReaderTest, WrongFooterSize) {
  auto test_wrong_footer_size = [this](int64_t wrong_size, const std::string& expected_prefix) {
    auto input_file = TestResources::CreateInputFile("v1/sample-metric-data-compressed-zstd.bin");
    auto length_result = input_file->length();
    ASSERT_TRUE(length_result.ok()) << length_result.error().message;
    
    auto reader = IcypuffReader(std::move(input_file), length_result.value(), wrong_size);
    auto blobs_result = reader.get_blobs();
    
    ASSERT_FALSE(blobs_result.ok());
    EXPECT_EQ(blobs_result.error().code, ErrorCode::kInvalidArgument);
    EXPECT_EQ(blobs_result.error().message.substr(0, expected_prefix.size()), expected_prefix);
  };

  const int64_t footer_size = SAMPLE_METRIC_DATA_COMPRESSED_ZSTD_FOOTER_SIZE;
  test_wrong_footer_size(footer_size - 1, "Invalid file: expected magic at offset");
  test_wrong_footer_size(footer_size + 1, "Invalid file: expected magic at offset");
  test_wrong_footer_size(footer_size - 10, "Invalid file: expected magic at offset");
  test_wrong_footer_size(footer_size + 10, "Invalid file: expected magic at offset");
  test_wrong_footer_size(footer_size - 10000, "Invalid footer size");
  test_wrong_footer_size(footer_size + 10000, "Invalid footer size");
}

TEST_F(IcypuffReaderTest, ReadMetricDataUncompressed) {
  auto input_file = TestResources::CreateInputFile("v1/sample-metric-data-uncompressed.bin");
  auto length_result = input_file->length();
  ASSERT_TRUE(length_result.ok()) << length_result.error().message;
  
  auto reader = IcypuffReader(std::move(input_file), length_result.value());
  auto blobs_result = reader.get_blobs();
  ASSERT_TRUE(blobs_result.ok()) << blobs_result.error().message;
  
  const auto& blobs = blobs_result.value();
  ASSERT_EQ(blobs.size(), 2);
  
  // Check file properties
  const auto& props = reader.properties();
  ASSERT_EQ(props.size(), 1);
  auto it = props.find("created-by");
  ASSERT_NE(it, props.end());
  EXPECT_EQ(it->second, "Test 1234");

  // Check first blob
  const auto& first_blob = blobs[0];
  EXPECT_EQ(first_blob->type(), "some-blob");
  ASSERT_EQ(first_blob->input_fields().size(), 1);
  EXPECT_EQ(first_blob->input_fields()[0], 1);
  EXPECT_EQ(first_blob->offset(), 4);
  EXPECT_EQ(first_blob->compression_codec(), std::nullopt);

  // Check second blob
  const auto& second_blob = blobs[1];
  EXPECT_EQ(second_blob->type(), "some-other-blob");
  ASSERT_EQ(second_blob->input_fields().size(), 1);
  EXPECT_EQ(second_blob->input_fields()[0], 2);
  EXPECT_EQ(second_blob->offset(), first_blob->offset() + first_blob->length());
  EXPECT_EQ(second_blob->compression_codec(), std::nullopt);

  // Read and verify blob data
  auto first_data = reader.read_blob(*first_blob);
  ASSERT_TRUE(first_data.ok()) << first_data.error().message;
  EXPECT_EQ(std::string(first_data.value().begin(), first_data.value().end()), "abcdefghi");

  auto second_data = reader.read_blob(*second_blob);
  ASSERT_TRUE(second_data.ok()) << second_data.error().message;
  std::string expected_data = "some blob \0 binary data 🤯 that is not very very very very very very long, is it?";
  EXPECT_EQ(std::string(second_data.value().begin(), second_data.value().end()), expected_data);
}

TEST_F(IcypuffReaderTest, ReadMetricDataCompressedZstd) {
  auto input_file = TestResources::CreateInputFile("v1/sample-metric-data-compressed-zstd.bin");
  auto length_result = input_file->length();
  ASSERT_TRUE(length_result.ok()) << length_result.error().message;
  
  auto reader = IcypuffReader(std::move(input_file), length_result.value());
  auto blobs_result = reader.get_blobs();
  ASSERT_TRUE(blobs_result.ok()) << blobs_result.error().message;
  
  const auto& blobs = blobs_result.value();
  ASSERT_EQ(blobs.size(), 2);
  
  // Check file properties
  const auto& props = reader.properties();
  ASSERT_EQ(props.size(), 1);
  auto it = props.find("created-by");
  ASSERT_NE(it, props.end());
  EXPECT_EQ(it->second, "Test 1234");

  // Check first blob
  const auto& first_blob = blobs[0];
  EXPECT_EQ(first_blob->type(), "some-blob");
  ASSERT_EQ(first_blob->input_fields().size(), 1);
  EXPECT_EQ(first_blob->input_fields()[0], 1);
  EXPECT_EQ(first_blob->offset(), 4);
  EXPECT_EQ(first_blob->compression_codec(), "zstd");

  // Check second blob
  const auto& second_blob = blobs[1];
  EXPECT_EQ(second_blob->type(), "some-other-blob");
  ASSERT_EQ(second_blob->input_fields().size(), 1);
  EXPECT_EQ(second_blob->input_fields()[0], 2);
  EXPECT_EQ(second_blob->offset(), first_blob->offset() + first_blob->length());
  EXPECT_EQ(second_blob->compression_codec(), "zstd");

  // Read and verify blob data
  auto first_data = reader.read_blob(*first_blob);
  ASSERT_TRUE(first_data.ok()) << first_data.error().message;
  EXPECT_EQ(std::string(first_data.value().begin(), first_data.value().end()), "abcdefghi");

  auto second_data = reader.read_blob(*second_blob);
  ASSERT_TRUE(second_data.ok()) << second_data.error().message;
  std::string expected_data = "some blob \0 binary data 🤯 that is not very very very very very very long, is it?";
  EXPECT_EQ(std::string(second_data.value().begin(), second_data.value().end()), expected_data);
}

TEST_F(IcypuffReaderTest, ValidateFooterSizeValue) {
  auto input_file = TestResources::CreateInputFile("v1/sample-metric-data-compressed-zstd.bin");
  auto length_result = input_file->length();
  ASSERT_TRUE(length_result.ok()) << length_result.error().message;
  
  auto reader = IcypuffReader(std::move(input_file), length_result.value(), 
                            SAMPLE_METRIC_DATA_COMPRESSED_ZSTD_FOOTER_SIZE);
  auto blobs_result = reader.get_blobs();
  ASSERT_TRUE(blobs_result.ok()) << blobs_result.error().message;
  
  const auto& props = reader.properties();
  ASSERT_EQ(props.size(), 1);
  auto it = props.find("created-by");
  ASSERT_NE(it, props.end());
  EXPECT_EQ(it->second, "Test 1234");
}

}  // namespace
}  // namespace icypuff 
