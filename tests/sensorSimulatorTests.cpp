#include <gtest/gtest.h>

#include "sensorSimulator.hpp"

#include <array>
#include <cstdint>

namespace {

TEST(SensorSimulator, AtTickZeroStartsNearOperatingBaselineWithDeterministicJitter) {
  const sdv::SensorSimulator simulator{};

  const auto reading = simulator.at(0);

  EXPECT_EQ(reading.sequenceNumber, 0U);
  EXPECT_EQ(reading.temperatureMilliCelsius, 41'811);
  EXPECT_EQ(reading.timestampMs, 1'700'000'000'000ULL);
}

TEST(SensorSimulator, ThermalCycleRaisesTemperatureThroughWarmUpPhase) {
  const sdv::SensorSimulator simulator{};

  const auto early = simulator.at(1);
  const auto midWarmUp = simulator.at(4);
  const auto nearPeak = simulator.at(12);

  EXPECT_EQ(early.temperatureMilliCelsius, 42'892);
  EXPECT_EQ(midWarmUp.temperatureMilliCelsius, 45'959);
  EXPECT_EQ(nearPeak.temperatureMilliCelsius, 49'854);
  EXPECT_GT(midWarmUp.temperatureMilliCelsius, early.temperatureMilliCelsius);
  EXPECT_GT(nearPeak.temperatureMilliCelsius, midWarmUp.temperatureMilliCelsius);
}

TEST(SensorSimulator, LoadSpikeAddsHeatDuringConfiguredWindow) {
  const sdv::SensorSimulator simulator{};

  const auto beforeSpike = simulator.at(27);
  const auto duringSpike = simulator.at(28);
  const auto afterSpike = simulator.at(36);

  EXPECT_EQ(duringSpike.temperatureMilliCelsius, 41'545);
  EXPECT_EQ(afterSpike.temperatureMilliCelsius, 33'940);
  EXPECT_GT(duringSpike.temperatureMilliCelsius, beforeSpike.temperatureMilliCelsius - 2'000);
  EXPECT_LT(afterSpike.temperatureMilliCelsius, duringSpike.temperatureMilliCelsius);
}

TEST(SensorSimulator, NextAdvancesTickAndKeepsTimestampsMonotonic) {
  sdv::SensorSimulator simulator{};

  const auto first = simulator.next();
  const auto second = simulator.next();

  EXPECT_EQ(first.sequenceNumber, 0U);
  EXPECT_EQ(second.sequenceNumber, 1U);
  EXPECT_EQ(first.timestampMs, 1'700'000'000'000ULL);
  EXPECT_EQ(second.timestampMs, 1'700'000'001'000ULL);
  EXPECT_EQ(simulator.currentTick(), 2U);
  EXPECT_NE(first.temperatureMilliCelsius, second.temperatureMilliCelsius);
}

TEST(SensorSimulator, AtIsPureAndDoesNotMutateInternalTick) {
  sdv::SensorSimulator simulator{};

  const auto peek = simulator.at(9);
  EXPECT_EQ(simulator.currentTick(), 0U);
  EXPECT_EQ(peek.sequenceNumber, 9U);
  EXPECT_EQ(peek.temperatureMilliCelsius, 49'535);
}

TEST(SensorSimulator, PublishedWireFormatMatchesIndependentExpectationForTick) {
  const sdv::SensorSimulator simulator{};
  const auto reading = simulator.at(7);
  const auto bytes = sdv::serialiseSensorReading(reading);

  const std::array<std::uint8_t, 16> expected{
      0x07, 0x00, 0x00, 0x00,
      0x21, 0xBD, 0x00, 0x00,
      0x58, 0x83, 0xE5, 0xCF, 0x8B, 0x01, 0x00, 0x00,
  };

  ASSERT_EQ(bytes.size(), expected.size());
  for (std::size_t index = 0; index < expected.size(); ++index) {
    EXPECT_EQ(bytes[index], expected[index]) << "Mismatch at byte index " << index;
  }

  const auto processed = sdv::processSensorPayload(bytes);
  ASSERT_TRUE(processed.has_value());
  EXPECT_EQ(processed->sequenceNumber, 7U);
  EXPECT_EQ(processed->temperatureMilliCelsius, 48'417);
  EXPECT_EQ(processed->timestampMs, 1'700'000'007'000ULL);
}

TEST(SensorSimulator, CustomAmplitudeScalesTheCycleSwing) {
  const sdv::SensorSimulator narrow{42'000, 1'000};
  const sdv::SensorSimulator wide{42'000, 10'000};

  const auto narrowPeak = narrow.at(12);
  const auto widePeak = wide.at(12);

  EXPECT_GT(widePeak.temperatureMilliCelsius, narrowPeak.temperatureMilliCelsius);
}

}  // namespace
