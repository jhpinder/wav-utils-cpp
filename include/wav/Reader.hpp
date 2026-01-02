#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>

namespace wav
{

  /**
   * @brief Basic WAV file reader for parsing RIFF/WAVE format files
   *
   * This is a minimal header-only implementation that reads WAV file headers
   * and provides access to audio metadata and sample data.
   *
   * Reference: wav-resources/WAVE File Format.html
   * Also see: riff-specs.pdf pages 56-65
   *
   * Usage example:
   *   wav::Reader reader("audio.wav");
   *   if (reader.open()) {
   *     std::cout << "Channels: " << reader.getNumChannels() << std::endl;
   *     std::cout << "Sample Rate: " << reader.getSampleRate() << std::endl;
   *   }
   */
  class Reader
  {
  public:
    Reader() : is_open_(false) {}

    explicit Reader(const std::string &filename)
        : filename_(filename), is_open_(false) {}

    /**
     * @brief Open and parse a WAV file
     * @return true if file was successfully opened and parsed
     */
    bool open(const std::string &filename)
    {
      filename_ = filename;
      return open();
    }

    /**
     * @brief Open and parse the WAV file specified in constructor
     * @return true if file was successfully opened and parsed
     */
    bool open()
    {
      if (filename_.empty())
      {
        return false;
      }

      std::ifstream file(filename_, std::ios::binary);
      if (!file.is_open())
      {
        return false;
      }

      // Read and verify RIFF header (12 bytes)
      // See wav-resources/WAVE File Format.html — RIFF chunk descriptor
      char riff_header[12];
      file.read(riff_header, 12);

      if (file.gcount() != 12)
      {
        return false;
      }

      // Verify "RIFF" chunk ID
      if (std::string(riff_header, 4) != "RIFF")
      {
        return false;
      }

      // Verify "WAVE" format
      if (std::string(riff_header + 8, 4) != "WAVE")
      {
        return false;
      }

      // Parse fmt chunk
      // See wav-resources/WAVE File Format.html — fmt chunk description
      if (!parseFmtChunk(file))
      {
        return false;
      }

      is_open_ = true;
      return true;
    }

    bool isOpen() const { return is_open_; }

    uint16_t getNumChannels() const { return num_channels_; }
    uint32_t getSampleRate() const { return sample_rate_; }
    uint16_t getBitsPerSample() const { return bits_per_sample_; }
    uint16_t getAudioFormat() const { return audio_format_; }

  private:
    bool parseFmtChunk(std::ifstream &file)
    {
      // Read chunk ID and size
      char chunk_id[4];
      uint32_t chunk_size;

      file.read(chunk_id, 4);
      file.read(reinterpret_cast<char *>(&chunk_size), 4);

      if (file.gcount() != 4)
      {
        return false;
      }

      // Verify this is the fmt chunk
      if (chunk_id[0] != 'f' || chunk_id[1] != 'm' ||
          chunk_id[2] != 't' || chunk_id[3] != ' ')
      {
        return false;
      }

      // Read fmt chunk data (minimum 16 bytes)
      // See wav-resources/WAVE File Format.html — fmt chunk format
      // Bytes are stored in little-endian format

      file.read(reinterpret_cast<char *>(&audio_format_), 2);    // Audio format (1 = PCM)
      file.read(reinterpret_cast<char *>(&num_channels_), 2);    // Number of channels
      file.read(reinterpret_cast<char *>(&sample_rate_), 4);     // Sample rate (Hz)
      file.read(reinterpret_cast<char *>(&byte_rate_), 4);       // Byte rate
      file.read(reinterpret_cast<char *>(&block_align_), 2);     // Block align
      file.read(reinterpret_cast<char *>(&bits_per_sample_), 2); // Bits per sample

      if (file.gcount() != 2)
      {
        return false;
      }

      // Skip any extra fmt chunk data
      if (chunk_size > 16)
      {
        file.seekg(chunk_size - 16, std::ios::cur);
      }

      return true;
    }

    std::string filename_;
    bool is_open_;

    // Format chunk data
    uint16_t audio_format_ = 0;
    uint16_t num_channels_ = 0;
    uint32_t sample_rate_ = 0;
    uint32_t byte_rate_ = 0;
    uint16_t block_align_ = 0;
    uint16_t bits_per_sample_ = 0;
  };

} // namespace wav
