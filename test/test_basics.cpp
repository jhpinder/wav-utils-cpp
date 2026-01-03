#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <wav/Reader.hpp>

TEST_CASE("file does not exist") {
  wav::Reader nonExistentFileReader("non_existent_file.wav");
  CHECK_FALSE(nonExistentFileReader.open());
}

TEST_CASE("invalid wav file") {
  // Create a temporary invalid WAV file
  const std::string invalidWavFilename = "invalid.wav";
  {
    std::ofstream ofs(invalidWavFilename, std::ios::binary);
    ofs << "INVALID DATA";
  }

  wav::Reader invalidWavReader(invalidWavFilename);
  CHECK_FALSE(invalidWavReader.open());

  // Clean up temporary file
  std::remove(invalidWavFilename.c_str());
}

TEST_CASE("valid wav file") {
  // Create a minimal valid WAV file (PCM, mono, 8-bit, 44100 Hz)
  const std::string validWavFilename = "valid.wav";
  {
    std::ofstream ofs(validWavFilename, std::ios::binary);
    // RIFF header
    ofs << "RIFF";
    uint32_t fileSize = 36; // Placeholder for file size
    ofs.write(reinterpret_cast<const char*>(&fileSize), 4);
    ofs << "WAVE";

    // fmt chunk
    ofs << "fmt ";
    uint32_t fmtChunkSize = 16;
    ofs.write(reinterpret_cast<const char*>(&fmtChunkSize), 4);
    uint16_t audioFormat = 1; // PCM
    ofs.write(reinterpret_cast<const char*>(&audioFormat), 2);
    uint16_t numChannels = 1; // Mono
    ofs.write(reinterpret_cast<const char*>(&numChannels), 2);
    uint32_t sampleRate = 44100;
    ofs.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = sampleRate * numChannels * 1; // 8-bit
    ofs.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = numChannels * 1; // 8-bit
    ofs.write(reinterpret_cast<const char*>(&blockAlign), 2);
    uint16_t bitsPerSample = 8;
    ofs.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data chunk
    ofs << "data";
    uint32_t dataChunkSize = 0; // No actual audio data
    ofs.write(reinterpret_cast<const char*>(&dataChunkSize), 4);
  }

  wav::Reader validWavReader(validWavFilename);
  CHECK(validWavReader.open());
  CHECK_EQ(validWavReader.getNumChannels(), 1);
  CHECK_EQ(validWavReader.getSampleRate(), 44100);
  CHECK_EQ(validWavReader.getBitsPerSample(), 8);
  CHECK_EQ(validWavReader.getAudioFormat(), 1); // PCM

  // Clean up temporary file
  std::remove(validWavFilename.c_str());
}