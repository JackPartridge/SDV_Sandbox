#include "vehicleSignalFrame.hpp"

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

auto makeVehicleSignalFrame(
    std::uint32_t sequenceNumber,
    const CanFrame& canFrame,
    std::uint64_t timestampMs) -> VehicleSignalFrame {
  return VehicleSignalFrame{
      .sequenceNumber = sequenceNumber,
      .engineRpm = canFrame.getEngineRpm(),
      .vehicleSpeed = canFrame.getVehicleSpeed(),
      .steeringAngle = canFrame.getSteeringAngle(),
      .sourceCanPayload = canFrame.getRawPayload(),
      .timestampMs = timestampMs,
  };
}

auto serialiseVehicleSignalFrame(const VehicleSignalFrame& frame) -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> buffer;
  buffer.reserve(vehicleSignalFrameByteSize);
  writeLittleEndian(buffer, frame.sequenceNumber);
  writeLittleEndian(buffer, frame.engineRpm);
  writeLittleEndian(buffer, frame.vehicleSpeed);
  writeLittleEndian(buffer, std::uint8_t{0});
  writeLittleEndian(buffer, frame.steeringAngle);
  writeLittleEndian(buffer, std::uint16_t{0});
  writeLittleEndian(buffer, frame.sourceCanPayload);
  writeLittleEndian(buffer, frame.timestampMs);
  return buffer;
}

auto deserialiseVehicleSignalFrame(std::span<const std::uint8_t> bytes)
    -> std::optional<VehicleSignalFrame> {
  if (bytes.size() != vehicleSignalFrameByteSize) {
    return std::nullopt;
  }
  return VehicleSignalFrame{
      .sequenceNumber = readLittleEndian<std::uint32_t>(bytes, 0),
      .engineRpm = readLittleEndian<std::uint16_t>(bytes, 4),
      .vehicleSpeed = readLittleEndian<std::uint8_t>(bytes, 6),
      .steeringAngle = readLittleEndian<std::uint16_t>(bytes, 8),
      .sourceCanPayload = readLittleEndian<std::uint64_t>(bytes, 12),
      .timestampMs = readLittleEndian<std::uint64_t>(bytes, 20),
  };
}

auto isPlausibleVehicleSignal(const VehicleSignalFrame& frame) -> bool {
  return frame.engineRpm <= 20'000
      && frame.vehicleSpeed <= 250
      && frame.steeringAngle <= 720;
}

auto processVehicleSignalPayload(std::span<const std::uint8_t> bytes)
    -> std::optional<VehicleSignalFrame> {
  const auto frame = deserialiseVehicleSignalFrame(bytes);
  if (!frame.has_value() || !isPlausibleVehicleSignal(*frame)) {
    return std::nullopt;
  }
  return frame;
}

auto operator==(const VehicleSignalFrame& left, const VehicleSignalFrame& right) -> bool {
  return left.sequenceNumber == right.sequenceNumber
      && left.engineRpm == right.engineRpm
      && left.vehicleSpeed == right.vehicleSpeed
      && left.steeringAngle == right.steeringAngle
      && left.sourceCanPayload == right.sourceCanPayload
      && left.timestampMs == right.timestampMs;
}

}  // namespace sdv
