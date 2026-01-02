#pragma once

#include <cstdint>
#include <string>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <filesystem>

namespace wav
{
  /**
   * @brief Format chunk data structure
   * See wav-resources/WAVE File Format.html — fmt chunk format
   */
  struct FmtChunk
  {
    uint16_t audio_format = 0;    // Audio format (1 = PCM)
    uint16_t num_channels = 0;    // Number of channels
    uint32_t sample_rate = 0;     // Sample rate (Hz)
    uint32_t byte_rate = 0;       // Byte rate
    uint16_t block_align = 0;     // Block align
    uint16_t bits_per_sample = 0; // Bits per sample
  };

  /**
   * @brief Data chunk information
   * See wav-resources/WAVE File Format.html — data chunk format
   */
  struct DataChunk
  {
    uint32_t size = 0;         // Size of data in bytes
    std::streampos offset = 0; // File offset where data begins
  };

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

      std::filesystem::path p = filename_;
      std::string abs_path;
      try
      {
        if (!p.is_absolute())
        {
          p = std::filesystem::absolute(p);
        }
        abs_path = p.string();
      }
      catch (const std::filesystem::filesystem_error &)
      {
        // If current_path() / getcwd fails (e.g., CWD was removed), fall back
        // to the user-provided filename instead of throwing.
        abs_path = filename_;
      }
      std::cout << "Opening file: " << abs_path << std::endl;

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

      // Parse all chunks in the file
      // WAV files may contain various chunks in any order (JUNK, fmt, data, fact, LIST, etc.)
      // See wav-resources/WAVE File Format.html — chunk structure
      bool found_fmt = false;

      while (file.good())
      {
        // Read chunk ID (4 bytes)
        char chunk_id[4];
        file.read(chunk_id, 4);

        if (file.gcount() != 4)
        {
          break; // End of file or read error
        }

        // Use string comparisons for chunk dispatch (simpler and clearer)
        std::string chunk_name(chunk_id, 4);

        if (chunk_name == "fmt ")
        {
          if (!readFmtChunk(file))
          {
            return false;
          }
          found_fmt = true;
        }
        else if (chunk_name == "data")
        {
          if (!readDataChunk(file))
          {
            return false;
          }
        }
        else if (chunk_name == "fact")
        {
          if (!readFactChunk(file))
          {
            return false;
          }
        }
        else if (chunk_name == "JUNK" || chunk_name == "LIST" || chunk_name == "INFO" ||
                 chunk_name == "cue " || chunk_name == "smpl" || chunk_name == "inst" ||
                 chunk_name == "bext" || chunk_name == "iXML")
        {
          // Known-but-not-actively-parsed chunks: skip their data
          if (!skipChunk(file))
          {
            return false;
          }
        }
        else
        {
          // Unknown/vendor-specific chunk: skip
          if (!skipChunk(file))
          {
            return false;
          }
        }
      }

      // fmt chunk is required for valid WAV file
      if (!found_fmt)
      {
        return false;
      }

      is_open_ = true;
      return true;
    }

    bool isOpen() const { return is_open_; }

    uint16_t getNumChannels() const { return fmt_.num_channels; }
    uint32_t getSampleRate() const { return fmt_.sample_rate; }
    uint16_t getBitsPerSample() const { return fmt_.bits_per_sample; }
    uint16_t getAudioFormat() const { return fmt_.audio_format; }

    const FmtChunk &getFmtChunk() const { return fmt_; }
    const DataChunk &getDataChunk() const { return data_; }

  private:
    /**
     * @brief Read fmt chunk data
     * @param file Input stream positioned right after the fmt chunk ID (at chunk size field)
     * See wav-resources/WAVE File Format.html — fmt chunk format (minimum 16 bytes)
     * Bytes are stored in little-endian format
     */
    bool readFmtChunk(std::ifstream &file)
    {
      uint32_t chunk_size;
      file.read(reinterpret_cast<char *>(&chunk_size), 4);

      if (file.gcount() != 4)
      {
        return false;
      }

      // Read fmt chunk fields
      file.read(reinterpret_cast<char *>(&fmt_.audio_format), 2);
      file.read(reinterpret_cast<char *>(&fmt_.num_channels), 2);
      file.read(reinterpret_cast<char *>(&fmt_.sample_rate), 4);
      file.read(reinterpret_cast<char *>(&fmt_.byte_rate), 4);
      file.read(reinterpret_cast<char *>(&fmt_.block_align), 2);
      file.read(reinterpret_cast<char *>(&fmt_.bits_per_sample), 2);

      if (file.gcount() != 2)
      {
        return false;
      }

      // Skip any extra fmt chunk data (e.g., extension for non-PCM formats)
      if (chunk_size > 16)
      {
        file.seekg(chunk_size - 16, std::ios::cur);
      }

      return true;
    }

    /**
     * @brief Read data chunk header
     * @param file Input stream positioned right after the data chunk ID
     * See wav-resources/WAVE File Format.html — data chunk format
     */
    bool readDataChunk(std::ifstream &file)
    {
      file.read(reinterpret_cast<char *>(&data_.size), 4);

      if (file.gcount() != 4)
      {
        return false;
      }

      // Store current position (start of actual sample data)
      data_.offset = file.tellg();

      // Skip the data for now (can be read later if needed)
      file.seekg(data_.size, std::ios::cur);

      return true;
    }

    /**
     * @brief Read fact chunk
     * @param file Input stream positioned right after the fact chunk ID
     * See wav-resources/WAVE File Format.html — fact chunk (for non-PCM formats)
     */
    bool readFactChunk(std::ifstream &file)
    {
      uint32_t chunk_size;
      file.read(reinterpret_cast<char *>(&chunk_size), 4);

      if (file.gcount() != 4)
      {
        return false;
      }

      // Skip fact chunk data (typically contains sample count for compressed formats)
      file.seekg(chunk_size, std::ios::cur);

      return true;
    }

    /**
     * @brief Skip unknown chunk
     * @param file Input stream positioned right after an unknown chunk ID
     */
    bool skipChunk(std::ifstream &file)
    {
      uint32_t chunk_size;
      file.read(reinterpret_cast<char *>(&chunk_size), 4);

      if (file.gcount() != 4)
      {
        return false;
      }

      // Skip the chunk data
      file.seekg(chunk_size, std::ios::cur);

      return true;
    }

    std::string filename_;
    bool is_open_;

    // Chunk data
    FmtChunk fmt_;
    DataChunk data_;
  };

} // namespace wav
