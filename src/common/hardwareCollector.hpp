#pragma once

#include "telemetryFrame.hpp"

#include <chrono>
#include <cstdint>

namespace sdv {

class HardwareCollector {
public:
  [[nodiscard]] auto sample(std::uint32_t sequenceNumber) -> TelemetryFrame;

private:
  [[nodiscard]] auto readCpuUtilisationMilliPercent() -> std::int32_t;
  [[nodiscard]] auto readMemoryUsedMilliPercent() -> std::int32_t;
  [[nodiscard]] auto readCpuTemperatureMilliCelsius() -> std::int32_t;
  [[nodiscard]] auto readLoadAverage1Milli() -> std::int32_t;
  [[nodiscard]] auto readSwapUsedMilliPercent() -> std::int32_t;
  [[nodiscard]] auto readProcessCount() -> std::uint32_t;
  auto readNetworkTotals(std::uint64_t& rxBytes, std::uint64_t& txBytes) -> void;
  auto readNetworkRates(std::uint32_t& rxBytesPerSec, std::uint32_t& txBytesPerSec) -> void;
  [[nodiscard]] auto readLogicalCpuCount() -> std::uint32_t;

  std::uint64_t previousIdle_{0};
  std::uint64_t previousTotal_{0};
  bool hasPreviousCpuSample_{false};

  std::uint64_t previousRxBytes_{0};
  std::uint64_t previousTxBytes_{0};
  std::chrono::steady_clock::time_point previousNetworkSampleAt_{};
  bool hasPreviousNetworkSample_{false};
};

}  // namespace sdv
