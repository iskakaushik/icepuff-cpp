#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "icypuff/file_metadata_parser.h"
#include "icypuff/file_metadata.h"
#include "icypuff/blob_metadata.h"

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

    const std::string expected_json = "{\n  \"blobs\": []\n}";
    EXPECT_EQ(json_result.value(), expected_json);

    // Test round-trip
    auto parsed_result = FileMetadataParser::FromJson(json_result.value());
    ASSERT_TRUE(parsed_result.ok());
    
    auto roundtrip_json = FileMetadataParser::ToJson(*parsed_result.value(), true);
    ASSERT_TRUE(roundtrip_json.ok());
    EXPECT_EQ(roundtrip_json.value(), expected_json);
}

TEST(FileMetadataParserTest, FileProperties) {
    // Test with single property
    {
        FileMetadataParams params;
        params.properties = {{"a property", "a property value"}};
        auto metadata_result = FileMetadata::Create(std::move(params));
        ASSERT_TRUE(metadata_result.ok());
        
        auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
        ASSERT_TRUE(json_result.ok());

        const std::string expected_json = "{\n"
            "  \"blobs\": [],\n"
            "  \"properties\": {\n"
            "    \"a property\": \"a property value\"\n"
            "  }\n"
            "}";
        EXPECT_EQ(json_result.value(), expected_json);

        // Test round-trip
        auto parsed_result = FileMetadataParser::FromJson(json_result.value());
        ASSERT_TRUE(parsed_result.ok());
        
        auto roundtrip_json = FileMetadataParser::ToJson(*parsed_result.value(), true);
        ASSERT_TRUE(roundtrip_json.ok());
        EXPECT_EQ(roundtrip_json.value(), expected_json);
    }

    // Test with multiple properties
    {
        FileMetadataParams params;
        params.properties = {
            {"a property", "a property value"},
            {"another one", "also with value"}
        };
        auto metadata_result = FileMetadata::Create(std::move(params));
        ASSERT_TRUE(metadata_result.ok());
        
        auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
        ASSERT_TRUE(json_result.ok());

        const std::string expected_json = "{\n"
            "  \"blobs\": [],\n"
            "  \"properties\": {\n"
            "    \"a property\": \"a property value\",\n"
            "    \"another one\": \"also with value\"\n"
            "  }\n"
            "}";
        EXPECT_EQ(json_result.value(), expected_json);
    }
}

TEST(FileMetadataParserTest, MissingBlobs) {
    auto result = FileMetadataParser::FromJson("{\"properties\": {}}");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error(), "Cannot parse missing field: blobs");
}

TEST(FileMetadataParserTest, BadBlobs) {
    auto result = FileMetadataParser::FromJson("{\"blobs\": {}}");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error(), "Cannot parse blobs from non-array: {}");
}

TEST(FileMetadataParserTest, BlobMetadata) {
    std::vector<std::unique_ptr<BlobMetadata>> blobs;
    
    {
        BlobMetadataParams params;
        params.type = "type-a";
        params.input_fields = {1};
        params.snapshot_id = 14;
        params.sequence_number = 3;
        params.offset = 4;
        params.length = 16;
        auto blob_result = BlobMetadata::Create(params);
        ASSERT_TRUE(blob_result.ok());
        blobs.push_back(std::move(blob_result.value()));
    }
    
    {
        BlobMetadataParams params;
        params.type = "type-bbb";
        params.input_fields = {2, 3, 4};
        params.snapshot_id = 77;
        params.sequence_number = 4;
        params.offset = INT64_MAX / 100;
        params.length = 79834;
        auto blob_result = BlobMetadata::Create(params);
        ASSERT_TRUE(blob_result.ok());
        blobs.push_back(std::move(blob_result.value()));
    }

    FileMetadataParams file_params;
    file_params.blobs = std::move(blobs);
    auto metadata_result = FileMetadata::Create(std::move(file_params));
    ASSERT_TRUE(metadata_result.ok());

    auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
    ASSERT_TRUE(json_result.ok());

    const std::string expected_json = "{\n"
        "  \"blobs\": [{\n"
        "    \"type\": \"type-a\",\n"
        "    \"fields\": [1],\n"
        "    \"snapshot-id\": 14,\n"
        "    \"sequence-number\": 3,\n"
        "    \"offset\": 4,\n"
        "    \"length\": 16\n"
        "  }, {\n"
        "    \"type\": \"type-bbb\",\n"
        "    \"fields\": [2, 3, 4],\n"
        "    \"snapshot-id\": 77,\n"
        "    \"sequence-number\": 4,\n"
        "    \"offset\": 92233720368547758,\n"
        "    \"length\": 79834\n"
        "  }]\n"
        "}";
    EXPECT_EQ(json_result.value(), expected_json);

    // Test round-trip
    auto parsed_result = FileMetadataParser::FromJson(json_result.value());
    ASSERT_TRUE(parsed_result.ok());
    
    auto roundtrip_json = FileMetadataParser::ToJson(*parsed_result.value(), true);
    ASSERT_TRUE(roundtrip_json.ok());
    EXPECT_EQ(roundtrip_json.value(), expected_json);
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
    
    auto blob_result = BlobMetadata::Create(blob_params);
    ASSERT_TRUE(blob_result.ok());

    FileMetadataParams file_params;
    file_params.blobs.push_back(std::move(blob_result.value()));
    auto metadata_result = FileMetadata::Create(std::move(file_params));
    ASSERT_TRUE(metadata_result.ok());

    auto json_result = FileMetadataParser::ToJson(*metadata_result.value(), true);
    ASSERT_TRUE(json_result.ok());

    const std::string expected_json = "{\n"
        "  \"blobs\": [{\n"
        "    \"type\": \"type-a\",\n"
        "    \"fields\": [1],\n"
        "    \"snapshot-id\": 14,\n"
        "    \"sequence-number\": 3,\n"
        "    \"offset\": 4,\n"
        "    \"length\": 16,\n"
        "    \"properties\": {\n"
        "      \"some key\": \"some value\"\n"
        "    }\n"
        "  }]\n"
        "}";
    EXPECT_EQ(json_result.value(), expected_json);

    // Test round-trip
    auto parsed_result = FileMetadataParser::FromJson(json_result.value());
    ASSERT_TRUE(parsed_result.ok());
    
    auto roundtrip_json = FileMetadataParser::ToJson(*parsed_result.value(), true);
    ASSERT_TRUE(roundtrip_json.ok());
    EXPECT_EQ(roundtrip_json.value(), expected_json);
}

TEST(FileMetadataParserTest, FieldNumberOutOfRange) {
    std::string json = "{\n"
        "  \"blobs\": [{\n"
        "    \"type\": \"type-a\",\n"
        "    \"fields\": [2147483648],\n"
        "    \"offset\": 4,\n"
        "    \"length\": 16\n"
        "  }]\n"
        "}";

    auto result = FileMetadataParser::FromJson(json);
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.error(), "Cannot parse integer from non-int value in fields: 2147483648");
}

}  // namespace
}  // namespace icypuff 