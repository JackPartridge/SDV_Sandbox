#pragma once

#include "sensorReading.hpp"

#include <cstdint>

namespace sdv {

class SensorSimulator {
public:
  SensorSimulator(
      std::int32_t baseTemperatureMilliCelsius = 42'000,
      std::int32_t cycleAmplitudeMilliCelsius = 8'000,
      std::uint64_t startTimestampMs = 1'700'000'000'000ULL,
      std::uint64_t intervalMs = 1'000);

  [[nodiscard]] auto at(std::uint32_t tick) const -> SensorReading;
  [[nodiscard]] auto next() -> SensorReading;
  [[nodiscard]] auto currentTick() const -> std::uint32_t;

private:
  [[nodiscard]] auto temperatureAt(std::uint32_t tick) const -> std::int32_t;

  std::int32_t baseTemperatureMilliCelsius_;
  std::int32_t cycleAmplitudeMilliCelsius_;
  std::uint64_t startTimestampMs_;
  std::uint64_t intervalMs_;
  std::uint32_t tick_{0};
};

}  // namespace sdv
