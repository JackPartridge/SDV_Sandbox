#pragma once

#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <vector>

namespace sdv {

struct TelemetryFrame {
  std::uint32_t sequenceNumber{};
  std::int32_t cpuUtilisationMilliPercent{};
  std::int32_t memoryUsedMilliPercent{};
  std::int32_t temperatureMilliCelsius{std::numeric_limits<std::int32_t>::min()};
  std::uint64_t timestampMs{};
  std::int32_t loadAverage1Milli{};
  std::int32_t swapUsedMilliPercent{};
  std::uint32_t processCount{};
  std::uint32_t networkRxBytesPerSec{};
  std::uint32_t networkTxBytesPerSec{};
  std::uint32_t logicalCpuCount{};
};

inline constexpr std::size_t telemetryFrameByteSize{48};
inline constexpr std::int32_t temperatureUnavailable{std::numeric_limits<std::int32_t>::min()};

[[nodiscard]] auto serialiseTelemetryFrame(const TelemetryFrame& frame) -> std::vector<std::uint8_t>;

[[nodiscard]] auto deserialiseTelemetryFrame(std::span<const std::uint8_t> bytes)
    -> std::optional<TelemetryFrame>;

[[nodiscard]] auto isPlausibleCpuUtilisation(std::int32_t cpuUtilisationMilliPercent) -> bool;
[[nodiscard]] auto isPlausibleMemoryUsed(std::int32_t memoryUsedMilliPercent) -> bool;
[[nodiscard]] auto isPlausibleTelemetryTemperature(std::int32_t temperatureMilliCelsius) -> bool;
[[nodiscard]] auto isPlausibleLoadAverage(std::int32_t loadAverage1Milli) -> bool;
[[nodiscard]] auto isPlausibleSwapUsed(std::int32_t swapUsedMilliPercent) -> bool;

[[nodiscard]] auto processTelemetryPayload(std::span<const std::uint8_t> bytes)
    -> std::optional<TelemetryFrame>;

[[nodiscard]] auto operator==(const TelemetryFrame& left, const TelemetryFrame& right) -> bool;

}  // namespace sdv
