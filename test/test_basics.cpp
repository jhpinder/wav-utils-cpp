#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include <wav/WavFileUtils.hpp>

TEST_CASE("valid file") {
  wav::WavFileUtils cueWavWavFileUtils("resources/loop-cue.wav");
  REQUIRE(cueWavWavFileUtils.open());
  CHECK_EQ(cueWavWavFileUtils.getNumChannels(), 1);
  CHECK_EQ(cueWavWavFileUtils.getSampleRate(), 96000);
  CHECK_EQ(cueWavWavFileUtils.getBitsPerSample(), 32);
  CHECK_EQ(cueWavWavFileUtils.getAudioFormat(), wav::AudioFormat::IEEE_FLOAT);
}

TEST_CASE("empty filename") {
  wav::WavFileUtils emptyFilenameWavFileUtils;
  CHECK_FALSE(emptyFilenameWavFileUtils.open());
}

TEST_CASE("file does not exist") {
  wav::WavFileUtils nonExistentFileWavFileUtils("non_existent_file.wav");
  CHECK_FALSE(nonExistentFileWavFileUtils.open());
}

TEST_CASE("invalid wav file") {
  // Create a temporary invalid WAV file
  const std::string invalidWavFilename = "invalid.wav";
  {
    std::ofstream ofs(invalidWavFilename, std::ios::binary);
    ofs << "INVALID DATA";
  }

  wav::WavFileUtils invalidWavWavFileUtils(invalidWavFilename);
  CHECK_FALSE(invalidWavWavFileUtils.open());

  // Clean up temporary file
  std::remove(invalidWavFilename.c_str());
}

TEST_CASE("test cue points") {
  wav::WavFileUtils cueWavWavFileUtils("resources/loop-cue.wav");
  REQUIRE(cueWavWavFileUtils.open());

  // Verify cue points
  const wav::CueChunk& cueChunk = cueWavWavFileUtils.getCueChunk();
  REQUIRE_EQ(cueChunk.numCuePoints, 1);
  CHECK_EQ(cueChunk.cuePoints[0].identifier, 0);
  CHECK_EQ(cueChunk.cuePoints[0].position, 0);
  CHECK_EQ(cueChunk.cuePoints[0].sampleOffset, 451437);
}

TEST_CASE("test data chunk") {
  wav::WavFileUtils dataWavFileUtilsFloat("resources/loop-cue.wav");
  wav::WavFileUtils dataWavFileUtils24B("resources/24b96khz128samples.wav");
  REQUIRE(dataWavFileUtilsFloat.open());
  REQUIRE(dataWavFileUtils24B.open());

  // Verify data chunk
  CHECK_EQ(dataWavFileUtilsFloat.getDataChunk().sampleDataInBytes.size(), 1834020);
  CHECK_EQ(dataWavFileUtils24B.getDataChunk().sampleDataInBytes.size(), 837);
}

TEST_CASE("test fact chunk") {
  wav::WavFileUtils factWavWavFileUtilsFloat("resources/loop-cue.wav");
  wav::WavFileUtils factWavWavFileUtils24B("resources/24b96khz128samples.wav");
  REQUIRE(factWavWavFileUtilsFloat.open());
  REQUIRE(factWavWavFileUtils24B.open());

  // Verify fact chunk
  CHECK_EQ(factWavWavFileUtilsFloat.getFactChunk().numSamplesPerChannel, 458505);
  CHECK_EQ(factWavWavFileUtils24B.getFactChunk().numSamplesPerChannel, 0);
}