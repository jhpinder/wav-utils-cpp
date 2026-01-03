#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace wav {

/**
 * @brief Audio format codes (see wav-resources/WAVE File Format.html — "fmt " chunk)
 */
enum class AudioFormat : uint16_t { PCM = 0x0001, IEEE_FLOAT = 0x0003 };

/**
 * @brief Format chunk data structure
 * See wav-resources/WAVE File Format.html — fmt chunk format
 */
struct FmtChunk {
  AudioFormat audioFormat = AudioFormat::PCM; // Audio format (1 = PCM)
  uint16_t numChannels = 0;                   // Number of channels
  uint32_t sampleRate = 0;                    // Sample rate (Hz)
  uint32_t byteRate = 0;                      // Byte rate
  uint16_t blockAlign = 0;                    // Block align
  uint16_t bitsPerSample = 0;                 // Bits per sample
};

/**
 * @brief Data chunk information with sample data
 * See wav-resources/WAVE File Format.html — data chunk format
 * Stores raw sample data in little-endian format as read from file.
 */
struct DataChunk {
  uint32_t size = 0;                          // Size of data in bytes
  std::streampos offset = 0;                  // File offset where data begins
  AudioFormat audioFormat = AudioFormat::PCM; // Audio format (1=PCM, 3=IEEE float)
  uint16_t bitsPerSample = 0;                 // Bits per sample (for format interpretation)
  std::vector<uint8_t> samples;               // Raw sample data in file byte order
};

/**
 * @brief Individual cue point structure
 * See wav-resources/WAVE File Format.html — cue chunk
 */
struct CuePoint {
  uint32_t identifier = 0;
  uint32_t position = 0;
  std::string fccChunk; // Should be "data"
  uint32_t chunkStart = 0;
  uint32_t blockStart = 0;
  uint32_t sampleOffset = 0;
};

/**
 * @brief Cue chunk information
 * See wav-resources/WAVE File Format.html — cue chunk
 */
struct CueChunk {
  long chunkSize = 0;
  long numCuePoints = 0;
  std::vector<CuePoint> cuePoints;
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
class Reader {
public:
  Reader() : isOpen_(false) {}

  explicit Reader(const std::string& filename) : filename_(filename), isOpen_(false) {}

  /**
   * @brief Open and parse a WAV file
   * @return true if file was successfully opened and parsed
   */
  bool open(const std::string& filename) {
    filename_ = filename;
    return open();
  }

  /**
   * @brief Open and parse the WAV file specified in constructor
   * @return true if file was successfully opened and parsed
   */
  bool open() {
    if (filename_.empty()) {
      return false;
    }

    std::filesystem::path p = filename_;
    std::string absPath;
    try {
      if (!p.is_absolute()) {
        p = std::filesystem::absolute(p);
      }
      absPath = p.string();
    } catch (const std::filesystem::filesystem_error&) {
      // If current_path() / getcwd fails (e.g., CWD was removed), fall back
      // to the user-provided filename instead of throwing.
      absPath = filename_;
    }
    std::cout << "Opening file: " << absPath << std::endl;

    std::ifstream file(filename_, std::ios::binary);
    if (!file.is_open()) {
      return false;
    }

    // Read and verify RIFF header (12 bytes)
    // See wav-resources/WAVE File Format.html — RIFF chunk descriptor
    char riffHeader[12];
    file.read(riffHeader, 12);

    if (file.gcount() != 12) {
      return false;
    }

    // Verify "RIFF" chunk ID
    if (std::string(riffHeader, 4) != "RIFF") {
      return false;
    }

    // Verify "WAVE" format
    if (std::string(riffHeader + 8, 4) != "WAVE") {
      return false;
    }

    // Parse all chunks in the file
    // WAV files may contain various chunks in any order (JUNK, fmt, data, fact, LIST, etc.)
    // See wav-resources/WAVE File Format.html — chunk structure
    bool foundFmtChunk = false;

    while (file.good()) {
      // Read chunk ID (4 bytes)
      char chunkId[4];
      file.read(chunkId, 4);

      if (file.gcount() != 4) {
        break; // End of file or read error
      }

      // Use string comparisons for chunk dispatch (simpler and clearer)
      std::string chunkName(chunkId, 4);

      if (chunkName == "fmt ") {
        if (!readFmtChunk(file)) {
          return false;
        }
        foundFmtChunk = true;
      } else if (chunkName == "data") {
        if (!readDataChunk(file)) {
          return false;
        }
      } else if (chunkName == "fact") {
        if (!readFactChunk(file)) {
          return false;
        }
      } else if (chunkName == "cue ") {
        if (!readCueChunk(file)) {
          return false;
        }
      } else if (chunkName == "JUNK" || chunkName == "LIST" || chunkName == "INFO" || chunkName == "smpl" ||
                 chunkName == "inst" || chunkName == "bext" || chunkName == "iXML") {
        // Known-but-not-actively-parsed chunks: skip their data
        if (!skipChunk(file)) {
          return false;
        }
      } else {
        // Unknown/vendor-specific chunk: skip
        if (!skipChunk(file)) {
          return false;
        }
      }
    }

    // fmt chunk is required for valid WAV file
    if (!foundFmtChunk) {
      return false;
    }

    isOpen_ = true;
    return true;
  }

  bool isOpen() const { return isOpen_; }

  uint16_t getNumChannels() const { return fmt_.numChannels; }
  uint32_t getSampleRate() const { return fmt_.sampleRate; }
  uint16_t getBitsPerSample() const { return fmt_.bitsPerSample; }
  AudioFormat getAudioFormat() const { return fmt_.audioFormat; }

  const FmtChunk& getFmtChunk() const { return fmt_; }
  const DataChunk& getDataChunk() const { return data_; }
  const CueChunk& getCueChunk() const { return cue_; }

private:
  /**
   * @brief Read fmt chunk data
   * @param file Input stream positioned right after the fmt chunk ID (at chunk size field)
   * See wav-resources/WAVE File Format.html — fmt chunk format (minimum 16 bytes)
   * Bytes are stored in little-endian format
   */
  bool readFmtChunk(std::ifstream& file) {
    uint32_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Read fmt chunk fields
    file.read(reinterpret_cast<char*>(&fmt_.audioFormat), 2);
    file.read(reinterpret_cast<char*>(&fmt_.numChannels), 2);
    file.read(reinterpret_cast<char*>(&fmt_.sampleRate), 4);
    file.read(reinterpret_cast<char*>(&fmt_.byteRate), 4);
    file.read(reinterpret_cast<char*>(&fmt_.blockAlign), 2);
    file.read(reinterpret_cast<char*>(&fmt_.bitsPerSample), 2);

    if (file.gcount() != 2) {
      return false;
    }

    // Skip any extra fmt chunk data (e.g., extension for non-PCM formats)
    if (chunkSize > 16) {
      file.seekg(chunkSize - 16, std::ios::cur);
    }

    return true;
  }

  /**
   * @brief Read data chunk and sample data
   * @param file Input stream positioned right after the data chunk ID
   * See wav-resources/WAVE File Format.html — data chunk format
   * Validates audio format before reading samples.
   */
  bool readDataChunk(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&data_.size), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Store format info for sample interpretation
    data_.audioFormat = fmt_.audioFormat;
    data_.bitsPerSample = fmt_.bitsPerSample;

    // Validate audio format (only PCM and IEEE float supported for now)
    if (data_.audioFormat != AudioFormat::PCM && data_.audioFormat != AudioFormat::IEEE_FLOAT) {
      std::cerr << "Error: Unsupported audio format 0x" << std::hex << static_cast<uint16_t>(data_.audioFormat)
                << std::dec << " (only PCM 0x0001 and IEEE float 0x0003 supported)\n";
      return false;
    }

    // Store current position (start of actual sample data)
    data_.offset = file.tellg();

    // Read sample data into vector
    if (data_.size > 0) {
      data_.samples.resize(data_.size);
      file.read(reinterpret_cast<char*>(data_.samples.data()), data_.size);

      if (file.gcount() != static_cast<std::streamsize>(data_.size)) {
        std::cerr << "Error: Failed to read complete data chunk. Expected " << data_.size << " bytes, got "
                  << file.gcount() << "\n";
        return false;
      }
    }

    return true;
  }

  /**
   * @brief Read fact chunk
   * @param file Input stream positioned right after the fact chunk ID
   * See wav-resources/WAVE File Format.html — fact chunk (for non-PCM formats)
   */
  bool readFactChunk(std::ifstream& file) {
    uint32_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Skip fact chunk data (typically contains sample count for compressed formats)
    file.seekg(chunkSize, std::ios::cur);

    return true;
  }

  /**
   * @brief Read cue chunk
   * @param file Input stream positioned right after the cue chunk ID
   * See wav-resources/WAVE File Format.html — cue chunk
   */
  bool readCueChunk(std::ifstream& file) {
    uint32_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Read number of cue points
    file.read(reinterpret_cast<char*>(&cue_.numCuePoints), 4);

    // Read each cue point
    for (long i = 0; i < cue_.numCuePoints; ++i) {
      CuePoint cuePoint;
      if (!readCuePoint(file, cuePoint)) {
        return false;
      }
      cue_.cuePoints.push_back(cuePoint);
    }

    return true;
  }

  /**
   * @brief Read individual cue point
   * @param file Input stream positioned right after a cue point ID
   * @param cuePoint CuePoint structure to fill
   */
  bool readCuePoint(std::ifstream& file, CuePoint& cuePoint) {
    file.read(reinterpret_cast<char*>(&cuePoint.identifier), 4);
    file.read(reinterpret_cast<char*>(&cuePoint.position), 4);

    char fccChunk[4];
    file.read(fccChunk, 4);
    cuePoint.fccChunk = std::string(fccChunk, 4);

    // We do not currently support cue points for other chunks than "data"
    if (cuePoint.fccChunk != "data") {
      return false;
    }

    file.read(reinterpret_cast<char*>(&cuePoint.chunkStart), 4);
    file.read(reinterpret_cast<char*>(&cuePoint.blockStart), 4);
    file.read(reinterpret_cast<char*>(&cuePoint.sampleOffset), 4);

    if (file.gcount() != 4) {
      return false;
    }

    return true;
  }

  /**
   * @brief Skip unknown chunk
   * @param file Input stream positioned right after an unknown chunk ID
   */
  bool skipChunk(std::ifstream& file) {
    uint32_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Skip the chunk data
    file.seekg(chunkSize, std::ios::cur);

    return true;
  }

  std::string filename_;
  bool isOpen_;

  // Chunk data
  FmtChunk fmt_;
  DataChunk data_;
  CueChunk cue_;
};

} // namespace wav
