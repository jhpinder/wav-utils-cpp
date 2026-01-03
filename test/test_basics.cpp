#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <wav/Reader.hpp>

TEST_CASE("valid file") {
  wav::Reader cueWavReader("resources/loop-cue.wav");
  REQUIRE(cueWavReader.open());
  CHECK_EQ(cueWavReader.getNumChannels(), 1);
  CHECK_EQ(cueWavReader.getSampleRate(), 96000);
  CHECK_EQ(cueWavReader.getBitsPerSample(), 32);
  CHECK_EQ(cueWavReader.getAudioFormat(), wav::AudioFormat::IEEE_FLOAT);
}

TEST_CASE("empty filename") {
  wav::Reader emptyFilenameReader;
  CHECK_FALSE(emptyFilenameReader.open());
}

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

TEST_CASE("test cue points") {
  wav::Reader cueWavReader("resources/loop-cue.wav");
  REQUIRE(cueWavReader.open());

  // Verify cue points
  const wav::CueChunk& cueChunk = cueWavReader.getCueChunk();
  REQUIRE_EQ(cueChunk.numCuePoints, 1);
  CHECK_EQ(cueChunk.cuePoints[0].identifier, 0);
  CHECK_EQ(cueChunk.cuePoints[0].position, 0);
  CHECK_EQ(cueChunk.cuePoints[0].sampleOffset, 451437);
}

TEST_CASE("test data chunk") {
  wav::Reader cueWavReader("resources/loop-cue.wav");
  REQUIRE(cueWavReader.open());

  // Verify data chunk
  const wav::DataChunk& dataChunk = cueWavReader.getDataChunk();
  CHECK_EQ(dataChunk.chunkSize, 1834020); // 458504 samples * 4 bytes/sample
  CHECK_EQ(dataChunk.sampleDataInBytes.size(), 57313);
}

TEST_CASE("test fact chunk") {
  wav::Reader cueWavReader("resources/loop-cue.wav");
  REQUIRE(cueWavReader.open());

  // Verify fact chunk
  const wav::FactChunk& factChunk = cueWavReader.getFactChunk();
  CHECK_EQ(factChunk.numSamplesPerChannel, 458505);
}