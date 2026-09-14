#include <gtest/gtest.h>

#include "telemetryFrame.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace {

TEST(TelemetryFrameSerialisation, EncodesKnownFrameIntoExactLittleEndianBytes) {
  const sdv::TelemetryFrame frame{
      .sequenceNumber = 7,
      .cpuUtilisationMilliPercent = 45'250,
      .memoryUsedMilliPercent = 62'500,
      .temperatureMilliCelsius = 57'125,
      .timestampMs = 1'700'000'005'000ULL,
      .loadAverage1Milli = 2'390,
      .swapUsedMilliPercent = 0,
      .processCount = 643,
      .networkRxBytesPerSec = 1'200,
      .networkTxBytesPerSec = 800,
      .logicalCpuCount = 16,
  };

  const auto bytes = sdv::serialiseTelemetryFrame(frame);
  const std::array<std::uint8_t, 48> expected{
      0x07, 0x00, 0x00, 0x00,
      0xC2, 0xB0, 0x00, 0x00,
      0x24, 0xF4, 0x00, 0x00,
      0x25, 0xDF, 0x00, 0x00,
      0x88, 0x7B, 0xE5, 0xCF, 0x8B, 0x01, 0x00, 0x00,
      0x56, 0x09, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x83, 0x02, 0x00, 0x00,
      0xB0, 0x04, 0x00, 0x00,
      0x20, 0x03, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
  };

  ASSERT_EQ(bytes.size(), expected.size());
  for (std::size_t index = 0; index < expected.size(); ++index) {
    EXPECT_EQ(bytes[index], expected[index]) << "Mismatch at byte index " << index;
  }
}

TEST(TelemetryFrameDeserialisation, RecoversExactFieldsFromGoldenBytes) {
  const std::array<std::uint8_t, 48> bytes{
      0x07, 0x00, 0x00, 0x00,
      0xC2, 0xB0, 0x00, 0x00,
      0x24, 0xF4, 0x00, 0x00,
      0x25, 0xDF, 0x00, 0x00,
      0x88, 0x7B, 0xE5, 0xCF, 0x8B, 0x01, 0x00, 0x00,
      0x56, 0x09, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
      0x83, 0x02, 0x00, 0x00,
      0xB0, 0x04, 0x00, 0x00,
      0x20, 0x03, 0x00, 0x00,
      0x10, 0x00, 0x00, 0x00,
  };

  const auto frame = sdv::deserialiseTelemetryFrame(bytes);
  ASSERT_TRUE(frame.has_value());
  EXPECT_EQ(frame->sequenceNumber, 7U);
  EXPECT_EQ(frame->cpuUtilisationMilliPercent, 45'250);
  EXPECT_EQ(frame->memoryUsedMilliPercent, 62'500);
  EXPECT_EQ(frame->temperatureMilliCelsius, 57'125);
  EXPECT_EQ(frame->timestampMs, 1'700'000'005'000ULL);
  EXPECT_EQ(frame->loadAverage1Milli, 2'390);
  EXPECT_EQ(frame->swapUsedMilliPercent, 0);
  EXPECT_EQ(frame->processCount, 643U);
  EXPECT_EQ(frame->networkRxBytesPerSec, 1'200U);
  EXPECT_EQ(frame->networkTxBytesPerSec, 800U);
  EXPECT_EQ(frame->logicalCpuCount, 16U);
}

TEST(TelemetryFrameValidation, RejectsImpossibleUtilisation) {
  sdv::TelemetryFrame frame{
      .sequenceNumber = 1,
      .cpuUtilisationMilliPercent = 150'000,
      .memoryUsedMilliPercent = 10'000,
      .temperatureMilliCelsius = sdv::temperatureUnavailable,
      .timestampMs = 1,
      .loadAverage1Milli = 100,
      .swapUsedMilliPercent = 0,
      .processCount = 1,
      .networkRxBytesPerSec = 0,
      .networkTxBytesPerSec = 0,
      .logicalCpuCount = 8,
  };
  const auto bytes = sdv::serialiseTelemetryFrame(frame);
  EXPECT_TRUE(sdv::deserialiseTelemetryFrame(bytes).has_value());
  EXPECT_FALSE(sdv::processTelemetryPayload(bytes).has_value());
}

TEST(TelemetryFrameValidation, AllowsUnavailableTemperature) {
  sdv::TelemetryFrame frame{
      .sequenceNumber = 1,
      .cpuUtilisationMilliPercent = 12'000,
      .memoryUsedMilliPercent = 40'000,
      .temperatureMilliCelsius = sdv::temperatureUnavailable,
      .timestampMs = 99,
      .loadAverage1Milli = 500,
      .swapUsedMilliPercent = 0,
      .processCount = 42,
      .networkRxBytesPerSec = 10,
      .networkTxBytesPerSec = 20,
      .logicalCpuCount = 4,
  };
  const auto bytes = sdv::serialiseTelemetryFrame(frame);
  const auto processed = sdv::processTelemetryPayload(bytes);
  ASSERT_TRUE(processed.has_value());
  EXPECT_EQ(processed->temperatureMilliCelsius, std::numeric_limits<std::int32_t>::min());
}

}  // namespace
