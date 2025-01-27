#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "icypuff/icypuff.h"
#include "icypuff/local_input_file.h"
#include "icypuff/local_output_file.h"

using json = nlohmann::json;

// Function to read and display puffin file info
void read_puffin(const std::string& filename) {
  auto input_file = std::make_unique<icypuff::LocalInputFile>(filename);
  auto length_result = input_file->length();
  if (!length_result.ok()) {
    std::cerr << "Failed to get file length: " << length_result.error().message << std::endl;
    return;
  }

  auto reader = icypuff::IcypuffReader(std::move(input_file), length_result.value());
  
  // Print file properties
  std::cout << "File Properties:" << std::endl;
  for (const auto& [key, value] : reader.properties()) {
    std::cout << "  " << key << ": " << value << std::endl;
  }

  // Get and print blob information
  auto blobs_result = reader.get_blobs();
  if (!blobs_result.ok()) {
    std::cerr << "Failed to read blobs: " << blobs_result.error().message << std::endl;
    return;
  }

  const auto& blobs = blobs_result.value();
  std::cout << "\nBlobs (" << blobs.size() << " total):" << std::endl;
  for (const auto& blob : blobs) {
    std::cout << "\nBlob Type: " << blob->type() << std::endl;
    std::cout << "  Offset: " << blob->offset() << std::endl;
    std::cout << "  Length: " << blob->length() << std::endl;
    if (blob->compression_codec()) {
      std::cout << "  Compression: " << *blob->compression_codec() << std::endl;
    }
    std::cout << "  Input Fields: ";
    for (int field : blob->input_fields()) {
      std::cout << field << " ";
    }
    std::cout << std::endl;

    // Read and display blob content if it's text
    auto data = reader.read_blob(*blob);
    if (data.ok()) {
      std::string content(data.value().begin(), data.value().end());
      if (content.length() < 1000) { // Only show if content is not too long
        std::cout << "  Content: " << content << std::endl;
      }
    }
  }
}

// Function to write a random quote to a puffin file
void write_quote(const std::string& filename) {
  // Some hardcoded quotes for demonstration
  std::vector<std::string> quotes = {
    "Be yourself; everyone else is already taken. - Oscar Wilde",
    "Two things are infinite: the universe and human stupidity; and I'm not sure about the universe. - Albert Einstein",
    "You only live once, but if you do it right, once is enough. - Mae West",
    "Be the change that you wish to see in the world. - Mahatma Gandhi",
    "In three words I can sum up everything I've learned about life: it goes on. - Robert Frost"
  };

  // Random quote selection
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, quotes.size() - 1);
  std::string selected_quote = quotes[dis(gen)];

  // Create output file
  auto output_file = std::make_unique<icypuff::LocalOutputFile>(filename);
  
  // Create writer
  auto writer_result = icypuff::Icypuff::write(std::move(output_file))
    .created_by("IcyPuff Demo App")
    .compress_blobs(icypuff::CompressionCodec::Zstd)
    .build();

  if (!writer_result.ok()) {
    std::cerr << "Failed to create writer: " << writer_result.error().message << std::endl;
    return;
  }

  auto writer = std::move(writer_result).value();

  // Write the quote as a blob
  auto write_result = writer->write_blob(
    reinterpret_cast<const uint8_t*>(selected_quote.data()),
    selected_quote.size(),
    "quote",
    std::vector<int>{1},  // input field
    1,                    // snapshot_id
    1                     // sequence_number
  );

  if (!write_result.ok()) {
    std::cerr << "Failed to write blob: " << write_result.error().message << std::endl;
    return;
  }

  // Close the writer
  auto close_result = writer->close();
  if (!close_result.ok()) {
    std::cerr << "Failed to close writer: " << close_result.error().message << std::endl;
    return;
  }

  std::cout << "Successfully wrote quote to " << filename << std::endl;
}

int main(int argc, char* argv[]) {
  try {
    cxxopts::Options options("icypuff-demo", "A demo application for reading and writing Puffin files");
    
    options.add_options()
      ("h,help", "Print usage")
      ("r,read", "Read a puffin file", cxxopts::value<std::string>())
      ("w,write", "Write a random quote to a puffin file", cxxopts::value<std::string>());

    auto result = options.parse(argc, argv);

    if (result.count("help") || (argc == 1)) {
      std::cout << options.help() << std::endl;
      return 0;
    }

    if (result.count("read")) {
      read_puffin(result["read"].as<std::string>());
    } else if (result.count("write")) {
      write_quote(result["write"].as<std::string>());
    } else {
      std::cout << options.help() << std::endl;
    }

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
