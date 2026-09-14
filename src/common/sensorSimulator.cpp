#include "sensorSimulator.hpp"

#include <cmath>

namespace sdv {
namespace {

constexpr std::uint32_t thermalCyclePeriodTicks{48};
constexpr std::uint32_t loadSpikeStartTick{28};
constexpr std::uint32_t loadSpikeEndTick{36};
constexpr std::int32_t loadSpikeMilliCelsius{3'500};

auto deterministicJitterMilliCelsius(std::uint32_t tick) -> std::int32_t {
  return static_cast<std::int32_t>((tick * 37u + 11u) % 401u) - 200;
}

}  // namespace

SensorSimulator::SensorSimulator(
    std::int32_t baseTemperatureMilliCelsius,
    std::int32_t cycleAmplitudeMilliCelsius,
    std::uint64_t startTimestampMs,
    std::uint64_t intervalMs)
    : baseTemperatureMilliCelsius_{baseTemperatureMilliCelsius},
      cycleAmplitudeMilliCelsius_{cycleAmplitudeMilliCelsius},
      startTimestampMs_{startTimestampMs},
      intervalMs_{intervalMs} {}

auto SensorSimulator::temperatureAt(std::uint32_t tick) const -> std::int32_t {
  const auto phase = static_cast<double>(tick % thermalCyclePeriodTicks)
      / static_cast<double>(thermalCyclePeriodTicks);
  const auto wave = std::sin(phase * 2.0 * 3.14159265358979323846);
  const auto cyclic = static_cast<std::int32_t>(
      std::lround(wave * static_cast<double>(cycleAmplitudeMilliCelsius_)));

  const auto positionInCycle = tick % thermalCyclePeriodTicks;
  const auto loadSpike = (positionInCycle >= loadSpikeStartTick && positionInCycle < loadSpikeEndTick)
      ? loadSpikeMilliCelsius
      : 0;

  return baseTemperatureMilliCelsius_ + cyclic + loadSpike + deterministicJitterMilliCelsius(tick);
}

auto SensorSimulator::at(std::uint32_t tick) const -> SensorReading {
  return SensorReading{
      .sequenceNumber = tick,
      .temperatureMilliCelsius = temperatureAt(tick),
      .timestampMs = startTimestampMs_ + static_cast<std::uint64_t>(tick) * intervalMs_,
  };
}

auto SensorSimulator::next() -> SensorReading {
  const auto reading = at(tick_);
  ++tick_;
  return reading;
}

auto SensorSimulator::currentTick() const -> std::uint32_t {
  return tick_;
}

}  // namespace sdv
