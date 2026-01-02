/**
 * @file basic_usage.cpp
 * @brief Tutorial example demonstrating basic WAV file reading
 *
 * This example shows how to use the wav::Reader class to open
 * a WAV file and read its metadata.
 */

#include <iostream>
#include <filesystem>
#include <wav/Reader.hpp>

/**
 * @brief Find a data file in common locations
 *
 * Searches for the file in:
 *   1. Current working directory (e.g., "wavs/loop-cue.wav")
 *   2. examples/ subdirectory (e.g., "examples/wavs/loop-cue.wav")
 *   3. Just the root with examples prefix
 *
 * This allows the example to work when run from different directories
 * (e.g., debug from build/examples/ or launch from repo root).
 */
static std::string findDataFile(const std::string &relative_path)
{
  namespace fs = std::filesystem;

  // Try 1: Direct path (e.g., from build/examples/basic_usage)
  if (fs::exists(relative_path))
  {
    return relative_path;
  }

  // Try 2: With examples/ prefix (e.g., from repo root)
  std::string with_examples = "examples/" + relative_path;
  if (fs::exists(with_examples))
  {
    return with_examples;
  }

  // Try 3: Assume we're in examples/ already (shouldn't reach here, but defensive)
  if (fs::exists(relative_path))
  {
    return relative_path;
  }

  // File not found; return the original path and let Reader report the error
  return relative_path;
}

int main(int argc, char *argv[])
{
  std::cout << "WAV Utils - Basic Usage Example\n";
  std::cout << "================================\n\n";
  std::string filename;
  // The wav::Reader class provides a simple interface for reading WAV files
  // It's a header-only library, so no linking is required!

  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <wav_file>\n";
    std::cout << "\nThis example demonstrates how to:\n";
    std::cout << "  1. Create a wav::Reader instance\n";
    std::cout << "  2. Open and parse a WAV file\n";
    std::cout << "  3. Read basic audio metadata\n";
    filename = findDataFile("wavs/loop-cue.wav");
  }
  else
  {
    filename = argv[1];
  }

  // Step 1: Create a Reader instance
  // You can either pass the filename to the constructor or use the default
  // constructor and call open() later
  wav::Reader reader(filename);

  // Step 2: Open the file
  // The open() method reads the RIFF header and fmt chunk
  // See wav-resources/WAVE File Format.html for the file format details
  if (!reader.open())
  {
    std::cerr << "Error: Could not open WAV file: " << filename << "\n";
    std::cerr << "Make sure the file exists and is a valid WAVE format file.\n";
    return 1;
  }

  std::cout << "Successfully opened: " << filename << "\n\n";

  // Step 3: Read metadata
  // The Reader class provides simple getters for all the fmt chunk data

  std::cout << "Audio Format Details:\n";
  std::cout << "--------------------\n";
  std::cout << "  Format:         " << reader.getAudioFormat()
            << (reader.getAudioFormat() == 1 ? " (PCM)" : " (Unknown)") << "\n";
  std::cout << "  Channels:       " << reader.getNumChannels() << "\n";
  std::cout << "  Sample Rate:    " << reader.getSampleRate() << " Hz\n";
  std::cout << "  Bits/Sample:    " << reader.getBitsPerSample() << "\n";

  // Print cue points if available
  const wav::CueChunk &cue = reader.getCueChunk();
  if (cue.num_cue_points > 0)
  {
    std::cout << "\nNumber of Cue Points: " << cue.num_cue_points << "\n";
    std::cout << "-----------\n";
    for (long i = 0; i < cue.num_cue_points; ++i)
    {
      const wav::CuePoint &point = cue.cue_points[i];
      std::cout << "  Cue Point " << i + 1 << ":\n";
      std::cout << "    Identifier:    " << point.identifier << "\n";
      std::cout << "    Position:      " << point.position << "\n";
      std::cout << "    Sample Offset: " << point.sample_offset << "\n";
    }
  }
  else
  {
    std::cout << "\nNo cue points found in this WAV file.\n";
  }

  // Calculate some derived values
  double duration_seconds = 0.0; // TODO: Read data chunk to calculate duration
  std::cout << "\nNote: This basic example only reads the file header.\n";
  std::cout << "Future versions will read sample data and calculate duration.\n";

  return 0;
}
