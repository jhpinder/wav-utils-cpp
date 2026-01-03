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
  wav::Reader dataReaderFloat("resources/loop-cue.wav");
  wav::Reader dataReader24B("resources/24b96khz128samples.wav");
  REQUIRE(dataReaderFloat.open());
  REQUIRE(dataReader24B.open());

  // Verify data chunk
  CHECK_EQ(dataReaderFloat.getDataChunk().sampleDataInBytes.size(), 1834020);
  CHECK_EQ(dataReader24B.getDataChunk().sampleDataInBytes.size(), 837);
}

TEST_CASE("test fact chunk") {
  wav::Reader factWavReaderFloat("resources/loop-cue.wav");
  wav::Reader factWavReader24B("resources/24b96khz128samples.wav");
  REQUIRE(factWavReaderFloat.open());
  REQUIRE(factWavReader24B.open());

  // Verify fact chunk
  CHECK_EQ(factWavReaderFloat.getFactChunk().numSamplesPerChannel, 458505);
  CHECK_EQ(factWavReader24B.getFactChunk().numSamplesPerChannel, 0);
}