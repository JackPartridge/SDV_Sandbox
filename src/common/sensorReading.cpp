#include "sensorReading.hpp"

#include <cstring>
#include <type_traits>

namespace sdv {
namespace {

constexpr std::int32_t minAutomotiveTemperatureMilliCelsius{-40'000};
constexpr std::int32_t maxAutomotiveTemperatureMilliCelsius{150'000};

template <typename T>
auto writeLittleEndian(std::vector<std::uint8_t>& buffer, T value) -> void {
  using Unsigned = std::make_unsigned_t<T>;
  auto bits = static_cast<Unsigned>(value);
  for (std::size_t index = 0; index < sizeof(T); ++index) {
    buffer.push_back(static_cast<std::uint8_t>((bits >> (8 * index)) & 0xFFU));
  }
}

template <typename T>
auto readLittleEndian(std::span<const std::uint8_t> bytes, std::size_t offset) -> T {
  using Unsigned = std::make_unsigned_t<T>;
  Unsigned bits{0};
  for (std::size_t index = 0; index < sizeof(T); ++index) {
    bits |= static_cast<Unsigned>(bytes[offset + index]) << (8 * index);
  }
  T result{};
  std::memcpy(&result, &bits, sizeof(T));
  return result;
}

}  // namespace

auto serialiseSensorReading(const SensorReading& reading) -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> buffer;
  buffer.reserve(sensorReadingByteSize);
  writeLittleEndian(buffer, reading.sequenceNumber);
  writeLittleEndian(buffer, reading.temperatureMilliCelsius);
  writeLittleEndian(buffer, reading.timestampMs);
  return buffer;
}

auto deserialiseSensorReading(std::span<const std::uint8_t> bytes) -> std::optional<SensorReading> {
  if (bytes.size() != sensorReadingByteSize) {
    return std::nullopt;
  }

  return SensorReading{
      .sequenceNumber = readLittleEndian<std::uint32_t>(bytes, 0),
      .temperatureMilliCelsius = readLittleEndian<std::int32_t>(bytes, 4),
      .timestampMs = readLittleEndian<std::uint64_t>(bytes, 8),
  };
}

auto isPlausibleAutomotiveTemperature(std::int32_t temperatureMilliCelsius) -> bool {
  return temperatureMilliCelsius >= minAutomotiveTemperatureMilliCelsius
      && temperatureMilliCelsius <= maxAutomotiveTemperatureMilliCelsius;
}

auto processSensorPayload(std::span<const std::uint8_t> bytes) -> std::optional<SensorReading> {
  const auto reading = deserialiseSensorReading(bytes);
  if (!reading.has_value()) {
    return std::nullopt;
  }
  if (!isPlausibleAutomotiveTemperature(reading->temperatureMilliCelsius)) {
    return std::nullopt;
  }
  return reading;
}

auto operator==(const SensorReading& left, const SensorReading& right) -> bool {
  return left.sequenceNumber == right.sequenceNumber
      && left.temperatureMilliCelsius == right.temperatureMilliCelsius
      && left.timestampMs == right.timestampMs;
}

}  // namespace sdv
