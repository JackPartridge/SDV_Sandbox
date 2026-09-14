#include "telemetryFrame.hpp"

#include <cstring>
#include <type_traits>

namespace sdv {
namespace {

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

auto serialiseTelemetryFrame(const TelemetryFrame& frame) -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> buffer;
  buffer.reserve(telemetryFrameByteSize);
  writeLittleEndian(buffer, frame.sequenceNumber);
  writeLittleEndian(buffer, frame.cpuUtilisationMilliPercent);
  writeLittleEndian(buffer, frame.memoryUsedMilliPercent);
  writeLittleEndian(buffer, frame.temperatureMilliCelsius);
  writeLittleEndian(buffer, frame.timestampMs);
  writeLittleEndian(buffer, frame.loadAverage1Milli);
  writeLittleEndian(buffer, frame.swapUsedMilliPercent);
  writeLittleEndian(buffer, frame.processCount);
  writeLittleEndian(buffer, frame.networkRxBytesPerSec);
  writeLittleEndian(buffer, frame.networkTxBytesPerSec);
  writeLittleEndian(buffer, frame.logicalCpuCount);
  return buffer;
}

auto deserialiseTelemetryFrame(std::span<const std::uint8_t> bytes) -> std::optional<TelemetryFrame> {
  if (bytes.size() != telemetryFrameByteSize) {
    return std::nullopt;
  }

  return TelemetryFrame{
      .sequenceNumber = readLittleEndian<std::uint32_t>(bytes, 0),
      .cpuUtilisationMilliPercent = readLittleEndian<std::int32_t>(bytes, 4),
      .memoryUsedMilliPercent = readLittleEndian<std::int32_t>(bytes, 8),
      .temperatureMilliCelsius = readLittleEndian<std::int32_t>(bytes, 12),
      .timestampMs = readLittleEndian<std::uint64_t>(bytes, 16),
      .loadAverage1Milli = readLittleEndian<std::int32_t>(bytes, 24),
      .swapUsedMilliPercent = readLittleEndian<std::int32_t>(bytes, 28),
      .processCount = readLittleEndian<std::uint32_t>(bytes, 32),
      .networkRxBytesPerSec = readLittleEndian<std::uint32_t>(bytes, 36),
      .networkTxBytesPerSec = readLittleEndian<std::uint32_t>(bytes, 40),
      .logicalCpuCount = readLittleEndian<std::uint32_t>(bytes, 44),
  };
}

auto isPlausibleCpuUtilisation(std::int32_t cpuUtilisationMilliPercent) -> bool {
  return cpuUtilisationMilliPercent >= 0 && cpuUtilisationMilliPercent <= 100'000;
}

auto isPlausibleMemoryUsed(std::int32_t memoryUsedMilliPercent) -> bool {
  return memoryUsedMilliPercent >= 0 && memoryUsedMilliPercent <= 100'000;
}

auto isPlausibleTelemetryTemperature(std::int32_t temperatureMilliCelsius) -> bool {
  if (temperatureMilliCelsius == temperatureUnavailable) {
    return true;
  }
  return temperatureMilliCelsius >= -40'000 && temperatureMilliCelsius <= 125'000;
}

auto isPlausibleLoadAverage(std::int32_t loadAverage1Milli) -> bool {
  return loadAverage1Milli >= 0 && loadAverage1Milli <= 1'000'000;
}

auto isPlausibleSwapUsed(std::int32_t swapUsedMilliPercent) -> bool {
  return swapUsedMilliPercent >= 0 && swapUsedMilliPercent <= 100'000;
}

auto processTelemetryPayload(std::span<const std::uint8_t> bytes) -> std::optional<TelemetryFrame> {
  const auto frame = deserialiseTelemetryFrame(bytes);
  if (!frame.has_value()) {
    return std::nullopt;
  }
  if (!isPlausibleCpuUtilisation(frame->cpuUtilisationMilliPercent)
      || !isPlausibleMemoryUsed(frame->memoryUsedMilliPercent)
      || !isPlausibleTelemetryTemperature(frame->temperatureMilliCelsius)
      || !isPlausibleLoadAverage(frame->loadAverage1Milli)
      || !isPlausibleSwapUsed(frame->swapUsedMilliPercent)) {
    return std::nullopt;
  }
  return frame;
}

auto operator==(const TelemetryFrame& left, const TelemetryFrame& right) -> bool {
  return left.sequenceNumber == right.sequenceNumber
      && left.cpuUtilisationMilliPercent == right.cpuUtilisationMilliPercent
      && left.memoryUsedMilliPercent == right.memoryUsedMilliPercent
      && left.temperatureMilliCelsius == right.temperatureMilliCelsius
      && left.timestampMs == right.timestampMs
      && left.loadAverage1Milli == right.loadAverage1Milli
      && left.swapUsedMilliPercent == right.swapUsedMilliPercent
      && left.processCount == right.processCount
      && left.networkRxBytesPerSec == right.networkRxBytesPerSec
      && left.networkTxBytesPerSec == right.networkTxBytesPerSec
      && left.logicalCpuCount == right.logicalCpuCount;
}

}  // namespace sdv
