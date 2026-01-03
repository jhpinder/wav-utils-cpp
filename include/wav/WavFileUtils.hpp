#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace wav {

typedef std::uint32_t chunkSize_t;

/**
 * @brief 4-byte chunk identifier
 */
struct Id {
  std::array<std::uint8_t, 4> b{};

  static constexpr Id fromChars(const char* chars) noexcept {
    return Id{{static_cast<std::uint8_t>(chars[0]), static_cast<std::uint8_t>(chars[1]),
               static_cast<std::uint8_t>(chars[2]), static_cast<std::uint8_t>(chars[3])}};
  }

  std::string toString() const { return std::string{char(b[0]), char(b[1]), char(b[2]), char(b[3])}; }

  friend bool operator==(Id x, Id y) noexcept { return x.b == y.b; }
  friend bool operator!=(Id x, Id y) noexcept { return !(x == y); }
};

/**
 * @brief Audio format codes (see wav-resources/WAVE File Format.html — "fmt " chunk)
 */
enum class AudioFormat : uint16_t { PCM = 0x0001, IEEE_FLOAT = 0x0003 };

/**
 * @brief Format chunk data structure
 * See wav-resources/WAVE File Format.html — fmt chunk format
 */
struct FmtChunk {
  chunkSize_t chunkSize = 0;                  // Size of the fmt chunk
  AudioFormat audioFormat = AudioFormat::PCM; // Audio format (1 = PCM)
  unsigned short numChannels = 0;             // Number of channels
  unsigned long sampleRate = 0;               // Sample rate (Hz)
  unsigned long avgBytesPerSec = 0;           // Byte rate
  unsigned short blockAlign = 0;              // Block align
  unsigned short bitsPerSample = 0;           // Bits per sample
};

/**
 * @brief Fact chunk structure
 * See wav-resources/WAVE File Format.html — fact chunk (for non-PCM formats)
 */
struct FactChunk {
  chunkSize_t chunkSize = 0;
  uint32_t numSamplesPerChannel = 0;
};

/**
 * @brief Data chunk information with sample data
 * See wav-resources/WAVE File Format.html — data chunk format
 * Stores raw sample data as read from file in a byte vector
 */
struct DataChunk {
  chunkSize_t chunkSize = 0;              // Size of data in bytes
  std::vector<uint8_t> sampleDataInBytes; // This is NOT framed samples, just raw bytes
};

/**
 * @brief Individual cue point structure
 * See wav-resources/WAVE File Format.html — cue chunk
 */
struct CuePoint {
  uint32_t identifier = 0;
  uint32_t position = 0;
  Id fccChunk; // Should be "data"
  uint32_t chunkStart = 0;
  uint32_t blockStart = 0;
  uint32_t sampleOffset = 0;
};

/**
 * @brief Cue chunk information
 * See wav-resources/WAVE File Format.html — cue chunk
 */
struct CueChunk {
  chunkSize_t chunkSize = 0;
  long numCuePoints = 0;
  std::vector<CuePoint> cuePoints;
};

/**
 * @brief LIST chunk header structure
 * See wav-resources/WAVE File Format.html — Associated Data List chunk
 */
struct ListHeader {
  chunkSize_t chunkSize = 0;
  std::string listType; // Should be "adtl"
};

/**
 * @brief Label chunk structure
 * See wav-resources/WAVE File Format.html — Associated Data List chunk
 */
struct LabelChunk {
  chunkSize_t chunkSize = 0;
  long cuePointId = 0; // Identifier of the associated cue point
  std::string text;
};

/**
 * @brief Note chunk structure
 * See wav-resources/WAVE File Format.html — Associated Data List chunk
 */
struct NoteChunk {
  chunkSize_t chunkSize = 0;
  long cuePointId = 0; // Identifier of the associated cue point
  std::string text;
};

/**
 * @brief Label Text chunk structure
 * See wav-resources/WAVE File Format.html — Associated Data List chunk
 */
struct LabelTextChunk {
  chunkSize_t chunkSize = 0;
  long cuePointId = 0; // Identifier of the associated cue point
  long sampleLength = 0;
  long purpose = 0;
  short country = 0;
  short language = 0;
  short dialect = 0;
  short codePage = 0;
  std::string text;
};

/**
 * @brief Sample loop structure
 * See wav-resources/WAVE File Format.html — sampler chunk
 */
struct SampleLoop {
  long cuePointId = 0;
  long type = 0;
  long start = 0;
  long end = 0;
  long fraction = 0;
  long playCount = 0;
};

/**
 * @brief Sampler chunk structure
 * See wav-resources/WAVE File Format.html — sampler chunk
 */
struct SamplerChunk {
  chunkSize_t chunkSize = 0;
  long manufacturer = 0;
  long product = 0;
  long samplePeriod = 0;
  long midiUnityNote = 0;
  long midiPitchFraction = 0;
  long smpteFormat = 0;
  long smpteOffset = 0;
  long numSampleLoops = 0;
  long samplerData = 0;
  std::vector<SampleLoop> sampleLoops;
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
 *   wav::WavFileUtils reader("audio.wav");
 *   if (reader.open()) {
 *     std::cout << "Channels: " << reader.getNumChannels() << std::endl;
 *     std::cout << "Sample Rate: " << reader.getSampleRate() << std::endl;
 *   }
 */
class WavFileUtils {
public:
  WavFileUtils() : isOpen_(false) {}

  explicit WavFileUtils(const std::string& filename) : filename_(filename), isOpen_(false) {}

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
    file.read(riffHeader, sizeof(riffHeader));

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
      char chunkIdChars[4];
      file.read(chunkIdChars, sizeof(chunkIdChars));

      if (file.gcount() != 4) {
        break; // End of file or read error
      }

      // Use string comparisons for chunk dispatch (simpler and clearer)
      wav::Id chunkId = wav::Id::fromChars(chunkIdChars);

      if (chunkId == wav::Id::fromChars("fmt ")) {
        if (!readFmtChunk(file)) {
          return false;
        }
        foundFmtChunk = true;
      } else if (chunkId == wav::Id::fromChars("data")) {
        if (!readDataChunk(file)) {
          return false;
        }
      } else if (chunkId == wav::Id::fromChars("fact")) {
        if (!readFactChunk(file)) {
          return false;
        }
      } else if (chunkId == wav::Id::fromChars("cue ")) {
        if (!readCueChunk(file)) {
          return false;
        }
      } else if (chunkId == wav::Id::fromChars("JUNK") || chunkId == wav::Id::fromChars("LIST") ||
                 chunkId == wav::Id::fromChars("INFO") || chunkId == wav::Id::fromChars("smpl") ||
                 chunkId == wav::Id::fromChars("inst") || chunkId == wav::Id::fromChars("bext") ||
                 chunkId == wav::Id::fromChars("iXML")) {
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
  /**
   * @brief Get raw sample data as a byte vector exactly as read from the file
   */
  std::vector<uint8_t> getRawSampleData() const { return data_.sampleDataInBytes; }

  const FmtChunk& getFmtChunk() const { return fmt_; }
  const DataChunk& getDataChunk() const { return data_; }
  const FactChunk& getFactChunk() const { return fact_; }
  const CueChunk& getCueChunk() const { return cue_; }

private:
  /**
   * @brief Read fmt chunk data
   * @param file Input stream positioned right after the fmt chunk ID (at chunk size field)
   * See wav-resources/WAVE File Format.html — fmt chunk format (minimum 16 bytes)
   * Bytes are stored in little-endian format
   */
  bool readFmtChunk(std::ifstream& file) {
    chunkSize_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Read fmt chunk fields
    file.read(reinterpret_cast<char*>(&fmt_.audioFormat), 2);
    file.read(reinterpret_cast<char*>(&fmt_.numChannels), 2);
    file.read(reinterpret_cast<char*>(&fmt_.sampleRate), 4);
    file.read(reinterpret_cast<char*>(&fmt_.avgBytesPerSec), 4);
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
   *
   * Note: chunkSize is the actual sample data bytes, not counting any pad byte.
   * If chunkSize is odd, a pad byte follows the data (to maintain even alignment).
   * Actual bytes to skip to next chunk = chunkSize + (chunkSize & 1)
   */
  bool readDataChunk(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&data_.chunkSize), sizeof(chunkSize_t));

    if (file.gcount() != 4) {
      return false;
    }

    // Validate audio format (only PCM and IEEE float supported for now)
    if (fmt_.audioFormat != AudioFormat::PCM && fmt_.audioFormat != AudioFormat::IEEE_FLOAT) {
      std::cerr << "Error: Unsupported audio format 0x" << std::hex << static_cast<uint16_t>(fmt_.audioFormat)
                << std::dec << " (only PCM 0x0001 and IEEE float 0x0003 supported)\n";
      return false;
    }

    // Read sample data into the appropriate typed container based on bitsPerSample
    if (data_.chunkSize > 0) {
      data_.sampleDataInBytes.resize(data_.chunkSize);
      file.read(reinterpret_cast<char*>(data_.sampleDataInBytes.data()), data_.chunkSize);

      if (file.gcount() != static_cast<std::streamsize>(data_.chunkSize)) {
        std::cerr << "Error: Failed to read complete data chunk. Expected " << data_.chunkSize << " bytes, got "
                  << file.gcount() << "\n";
        return false;
      }
    }

    // Skip any pad byte if chunkSize is odd (maintains even alignment per RIFF spec)
    if (data_.chunkSize & 1) {
      file.seekg(1, std::ios::cur);
    }

    return true;
  }

  /**
   * @brief Read fact chunk
   * @param file Input stream positioned right after the fact chunk ID
   * See wav-resources/WAVE File Format.html — fact chunk (for non-PCM formats)
   */
  bool readFactChunk(std::ifstream& file) {
    chunkSize_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize_t));

    if (file.gcount() != 4) {
      return false;
    }

    file.read(reinterpret_cast<char*>(&fact_.numSamplesPerChannel), 4);

    if (file.gcount() != 4) {
      return false;
    }

    return true;
  }

  /**
   * @brief Read cue chunk
   * @param file Input stream positioned right after the cue chunk ID
   * See wav-resources/WAVE File Format.html — cue chunk
   */
  bool readCueChunk(std::ifstream& file) {
    chunkSize_t chunkSize;
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
    cuePoint.fccChunk = Id::fromChars(fccChunk);

    // We do not currently support cue points for other chunks than "data"
    if (cuePoint.fccChunk != Id::fromChars("data")) {
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

  bool readAdtlChunk(std::ifstream& file) {
    chunkSize_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    return true;
  }

  /**
   * @brief Skip unknown chunk
   * @param file Input stream positioned right after an unknown chunk ID
   * Note: Accounts for pad byte if chunkSize is odd (maintains even alignment).
   * Bytes to skip = chunkSize + (chunkSize & 1)
   */
  bool skipChunk(std::ifstream& file) {
    chunkSize_t chunkSize;
    file.read(reinterpret_cast<char*>(&chunkSize), 4);

    if (file.gcount() != 4) {
      return false;
    }

    // Skip the chunk data
    file.seekg(chunkSize, std::ios::cur);

    // Skip pad byte if chunkSize is odd (maintains even alignment per RIFF spec)
    if (chunkSize & 1) {
      file.seekg(1, std::ios::cur);
    }

    return true;
  }

  std::string filename_;
  bool isOpen_;

  // Chunk data
  FmtChunk fmt_;
  DataChunk data_;
  FactChunk fact_;
  CueChunk cue_;
};

} // namespace wav
