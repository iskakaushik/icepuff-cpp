#include "icypuff/file_metadata_parser.h"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "icypuff/blob_metadata.h"
#include "icypuff/file_metadata.h"

namespace icypuff {
namespace {

TEST(FileMetadataParserTest, InvalidJson) {
  // Test null/empty
  auto result = FileMetadataParser::FromJson("");
  EXPECT_FALSE(result.ok());
  EXPECT_TRUE(result.error().message.find("end-of-input") != std::string::npos);

  // Test incomplete JSON
  result = FileMetadataParser::FromJson("{");
  EXPECT_FALSE(result.ok());
  EXPECT_TRUE(result.error().message.find("end-of-input") != std::string::npos);

  result = FileMetadataParser::FromJson("{\"blobs\": []");
  EXPECT_FALSE(result.ok());
  EXPECT_TRUE(result.error().message.find("end-of-input") != std::string::npos);
}

TEST(FileMetadataParserTest, MinimalFileMetadata) {
  FileMetadataParams params;
  auto metadata_result = FileMetadata::Create(std::move(params));
  ASSERT_TRUE(metadata_result.ok());

  auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
  ASSERT_TRUE(json_result.ok());

  auto parsed_json = nlohmann::json::parse(json_result.value());
  auto expected_json = nlohmann::json::parse(R"({
        "blobs": []
    })");
  EXPECT_EQ(parsed_json, expected_json);

  // Test round-trip
  auto parsed_result = FileMetadataParser::FromJson(json_result.value());
  ASSERT_TRUE(parsed_result.ok());

  auto roundtrip_json =
      FileMetadataParser::ToJson(*parsed_result.value(), true);
  ASSERT_TRUE(roundtrip_json.ok());
  auto roundtrip_parsed = nlohmann::json::parse(roundtrip_json.value());
  EXPECT_EQ(roundtrip_parsed, expected_json);
}

TEST(FileMetadataParserTest, FileProperties) {
  // Test with single property
  {
    FileMetadataParams params;
    params.properties = {{"a property", "a property value"}};
    auto metadata_result = FileMetadata::Create(std::move(params));
    ASSERT_TRUE(metadata_result.ok());

    auto json_result =
        FileMetadataParser::ToJson(*metadata_result.value(), true);
    ASSERT_TRUE(json_result.ok());

    // Parse the generated JSON and expected JSON to compare data instead of
    // formatting
    auto parsed_json = nlohmann::json::parse(json_result.value());
    auto expected_json = nlohmann::json::parse(R"({
            "blobs": [],
            "properties": {
                "a property": "a property value"
            }
        })");
    EXPECT_EQ(parsed_json, expected_json);

    // Test round-trip
    auto parsed_result = FileMetadataParser::FromJson(json_result.value());
    ASSERT_TRUE(parsed_result.ok());

    auto roundtrip_json =
        FileMetadataParser::ToJson(*parsed_result.value(), true);
    ASSERT_TRUE(roundtrip_json.ok());
    auto roundtrip_parsed = nlohmann::json::parse(roundtrip_json.value());
    EXPECT_EQ(roundtrip_parsed, expected_json);
  }

  // Test with multiple properties
  {
    FileMetadataParams params;
    params.properties = {{"a property", "a property value"},
                         {"another one", "also with value"}};
    auto metadata_result = FileMetadata::Create(std::move(params));
    ASSERT_TRUE(metadata_result.ok());

    auto json_result =
        FileMetadataParser::ToJson(*metadata_result.value(), true);
    ASSERT_TRUE(json_result.ok());

    auto parsed_json = nlohmann::json::parse(json_result.value());
    auto expected_json = nlohmann::json::parse(R"({
            "blobs": [],
            "properties": {
                "a property": "a property value",
                "another one": "also with value"
            }
        })");
    EXPECT_EQ(parsed_json, expected_json);
  }
}

TEST(FileMetadataParserTest, MissingBlobs) {
  auto result = FileMetadataParser::FromJson("{\"properties\": {}}");
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error().message, "Cannot parse missing field: blobs");
}

TEST(FileMetadataParserTest, BadBlobs) {
  auto result = FileMetadataParser::FromJson("{\"blobs\": {}}");
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error().message, "Cannot parse blobs from non-array: {}");
}

TEST(FileMetadataParserTest, BlobMetadata) {
  FileMetadataParams file_params;

  {
    BlobMetadataParams params;
    params.type = "type-a";
    params.input_fields = {1};
    params.snapshot_id = 14;
    params.sequence_number = 3;
    params.offset = 4;
    params.length = 16;
    auto blob = BlobMetadata::Create(params);
    ASSERT_TRUE(blob.ok());
    file_params.blobs.push_back(std::move(blob).value());
  }

  {
    BlobMetadataParams params;
    params.type = "type-bbb";
    params.input_fields = {2, 3, 4};
    params.snapshot_id = 77;
    params.sequence_number = 4;
    params.offset = INT64_MAX / 100;
    params.length = 79834;
    auto blob = BlobMetadata::Create(params);
    ASSERT_TRUE(blob.ok());
    file_params.blobs.push_back(std::move(blob).value());
  }

  auto metadata_result = FileMetadata::Create(std::move(file_params));
  ASSERT_TRUE(metadata_result.ok());

  auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
  ASSERT_TRUE(json_result.ok());

  auto parsed_json = nlohmann::json::parse(json_result.value());
  auto expected_json = nlohmann::json::parse(R"({
        "blobs": [{
            "type": "type-a",
            "fields": [1],
            "snapshot-id": 14,
            "sequence-number": 3,
            "offset": 4,
            "length": 16
        }, {
            "type": "type-bbb",
            "fields": [2, 3, 4],
            "snapshot-id": 77,
            "sequence-number": 4,
            "offset": 92233720368547758,
            "length": 79834
        }]
    })");
  EXPECT_EQ(parsed_json, expected_json);

  // Test round-trip
  auto parsed_result = FileMetadataParser::FromJson(json_result.value());
  ASSERT_TRUE(parsed_result.ok());

  auto roundtrip_json =
      FileMetadataParser::ToJson(*parsed_result.value(), true);
  ASSERT_TRUE(roundtrip_json.ok());
  auto roundtrip_parsed = nlohmann::json::parse(roundtrip_json.value());
  EXPECT_EQ(roundtrip_parsed, expected_json);
}

TEST(FileMetadataParserTest, BlobProperties) {
  BlobMetadataParams blob_params;
  blob_params.type = "type-a";
  blob_params.input_fields = {1};
  blob_params.snapshot_id = 14;
  blob_params.sequence_number = 3;
  blob_params.offset = 4;
  blob_params.length = 16;
  blob_params.properties = {{"some key", "some value"}};

  auto blob = BlobMetadata::Create(blob_params);
  ASSERT_TRUE(blob.ok());

  FileMetadataParams file_params;
  file_params.blobs.push_back(std::move(blob).value());
  auto metadata_result = FileMetadata::Create(std::move(file_params));
  ASSERT_TRUE(metadata_result.ok());

  auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
  ASSERT_TRUE(json_result.ok());

  auto parsed_json = nlohmann::json::parse(json_result.value());
  auto expected_json = nlohmann::json::parse(R"({
        "blobs": [{
            "type": "type-a",
            "fields": [1],
            "snapshot-id": 14,
            "sequence-number": 3,
            "offset": 4,
            "length": 16,
            "properties": {
                "some key": "some value"
            }
        }]
    })");
  EXPECT_EQ(parsed_json, expected_json);

  // Test round-trip
  auto parsed_result = FileMetadataParser::FromJson(json_result.value());
  ASSERT_TRUE(parsed_result.ok());

  auto roundtrip_json =
      FileMetadataParser::ToJson(*parsed_result.value(), true);
  ASSERT_TRUE(roundtrip_json.ok());
  auto roundtrip_parsed = nlohmann::json::parse(roundtrip_json.value());
  EXPECT_EQ(roundtrip_parsed, expected_json);
}

TEST(FileMetadataParserTest, FieldNumberOutOfRange) {
  auto result = FileMetadataParser::FromJson(R"({
        "blobs": [{
            "type": "type-a",
            "fields": [2147483648],
            "offset": 4,
            "length": 16
        }]
    })");
  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.error().message,
            "Cannot parse integer from non-int value in fields: 2147483648");
}

}  // namespace
}  // namespace icypuff