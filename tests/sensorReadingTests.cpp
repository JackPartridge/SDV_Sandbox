#include <gtest/gtest.h>

#include "sensorReading.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace {

TEST(SensorReadingSerialisation, EncodesKnownReadingIntoExactLittleEndianBytes) {
  const sdv::SensorReading reading{
      .sequenceNumber = 7,
      .temperatureMilliCelsius = 21'500,
      .timestampMs = 1'700'000'005'000ULL,
  };

  const auto bytes = sdv::serialiseSensorReading(reading);

  const std::array<std::uint8_t, 16> expected{
      0x07, 0x00, 0x00, 0x00,
      0xFC, 0x53, 0x00, 0x00,
      0x88, 0x7B, 0xE5, 0xCF, 0x8B, 0x01, 0x00, 0x00,
  };

  ASSERT_EQ(bytes.size(), expected.size());
  for (std::size_t index = 0; index < expected.size(); ++index) {
    EXPECT_EQ(bytes[index], expected[index]) << "Mismatch at byte index " << index;
  }
}

TEST(SensorReadingSerialisation, EncodesNegativeTemperatureWithTwoComplementLittleEndian) {
  const sdv::SensorReading reading{
      .sequenceNumber = 1,
      .temperatureMilliCelsius = -40'000,
      .timestampMs = 0,
  };

  const auto bytes = sdv::serialiseSensorReading(reading);

  ASSERT_EQ(bytes.size(), 16U);
  EXPECT_EQ(bytes[0], 0x01);
  EXPECT_EQ(bytes[1], 0x00);
  EXPECT_EQ(bytes[2], 0x00);
  EXPECT_EQ(bytes[3], 0x00);
  EXPECT_EQ(bytes[4], 0xC0);
  EXPECT_EQ(bytes[5], 0x63);
  EXPECT_EQ(bytes[6], 0xFF);
  EXPECT_EQ(bytes[7], 0xFF);
  for (std::size_t index = 8; index < 16; ++index) {
    EXPECT_EQ(bytes[index], 0x00) << "Timestamp byte should be zero at " << index;
  }
}

TEST(SensorReadingDeserialisation, RecoversExactFieldsFromGoldenByteSequence) {
  const std::array<std::uint8_t, 16> bytes{
      0x07, 0x00, 0x00, 0x00,
      0xFC, 0x53, 0x00, 0x00,
      0x88, 0x7B, 0xE5, 0xCF, 0x8B, 0x01, 0x00, 0x00,
  };

  const auto reading = sdv::deserialiseSensorReading(bytes);

  ASSERT_TRUE(reading.has_value());
  EXPECT_EQ(reading->sequenceNumber, 7U);
  EXPECT_EQ(reading->temperatureMilliCelsius, 21'500);
  EXPECT_EQ(reading->timestampMs, 1'700'000'005'000ULL);
}

TEST(SensorReadingDeserialisation, RejectsPayloadsWithWrongLength) {
  const std::vector<std::uint8_t> tooShort(15, 0x00);
  const std::vector<std::uint8_t> tooLong(17, 0x00);

  EXPECT_FALSE(sdv::deserialiseSensorReading(tooShort).has_value());
  EXPECT_FALSE(sdv::deserialiseSensorReading(tooLong).has_value());
  EXPECT_FALSE(sdv::deserialiseSensorReading({}).has_value());
}

TEST(SensorReadingRoundTrip, PreservesExtremeButValidFieldValues) {
  const sdv::SensorReading original{
      .sequenceNumber = 0xFFFFFFFFu,
      .temperatureMilliCelsius = -40'000,
      .timestampMs = 0x0123456789ABCDEFULL,
  };

  const auto bytes = sdv::serialiseSensorReading(original);
  const auto restored = sdv::deserialiseSensorReading(bytes);

  ASSERT_TRUE(restored.has_value());
  EXPECT_EQ(*restored, original);
}

TEST(TemperaturePlausibility, AcceptsAutomotiveRangeBoundaries) {
  EXPECT_TRUE(sdv::isPlausibleAutomotiveTemperature(-40'000));
  EXPECT_TRUE(sdv::isPlausibleAutomotiveTemperature(150'000));
  EXPECT_TRUE(sdv::isPlausibleAutomotiveTemperature(0));
}

TEST(TemperaturePlausibility, RejectsValuesOutsideAutomotiveRange) {
  EXPECT_FALSE(sdv::isPlausibleAutomotiveTemperature(-40'001));
  EXPECT_FALSE(sdv::isPlausibleAutomotiveTemperature(150'001));
  EXPECT_FALSE(sdv::isPlausibleAutomotiveTemperature(1'000'000));
}

TEST(ProcessSensorPayload, AcceptsValidWirePayloadAndReturnsDecodedReading) {
  const std::array<std::uint8_t, 16> bytes{
      0x02, 0x00, 0x00, 0x00,
      0x10, 0x27, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };

  const auto reading = sdv::processSensorPayload(bytes);

  ASSERT_TRUE(reading.has_value());
  EXPECT_EQ(reading->sequenceNumber, 2U);
  EXPECT_EQ(reading->temperatureMilliCelsius, 10'000);
  EXPECT_EQ(reading->timestampMs, 0ULL);
}

TEST(ProcessSensorPayload, RejectsStructurallyValidPayloadWithImpossibleTemperature) {
  const sdv::SensorReading impossible{
      .sequenceNumber = 3,
      .temperatureMilliCelsius = 200'000,
      .timestampMs = 42,
  };
  const auto bytes = sdv::serialiseSensorReading(impossible);

  EXPECT_TRUE(sdv::deserialiseSensorReading(bytes).has_value());
  EXPECT_FALSE(sdv::processSensorPayload(bytes).has_value());
}

TEST(ProcessSensorPayload, RejectsTruncatedPayloadEvenIfPrefixLooksValid) {
  const std::array<std::uint8_t, 8> truncated{
      0x01, 0x00, 0x00, 0x00,
      0x10, 0x27, 0x00, 0x00,
  };

  EXPECT_FALSE(sdv::processSensorPayload(truncated).has_value());
}

}  // namespace
