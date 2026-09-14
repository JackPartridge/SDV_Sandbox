#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace sdv {

struct SensorReading {
  std::uint32_t sequenceNumber{};
  std::int32_t temperatureMilliCelsius{};
  std::uint64_t timestampMs{};
};

inline constexpr std::size_t sensorReadingByteSize{16};

[[nodiscard]] auto serialiseSensorReading(const SensorReading& reading) -> std::vector<std::uint8_t>;

[[nodiscard]] auto deserialiseSensorReading(std::span<const std::uint8_t> bytes)
    -> std::optional<SensorReading>;

[[nodiscard]] auto isPlausibleAutomotiveTemperature(std::int32_t temperatureMilliCelsius) -> bool;

[[nodiscard]] auto processSensorPayload(std::span<const std::uint8_t> bytes)
    -> std::optional<SensorReading>;

[[nodiscard]] auto operator==(const SensorReading& left, const SensorReading& right) -> bool;

}  // namespace sdv
