#include "canFrame.hpp"

#include <stdexcept>

namespace sdv {

CanFrame::CanFrame(std::uint64_t rawPayload) : rawPayload_{rawPayload} {}

auto CanFrame::getRawPayload() const -> std::uint64_t {
  return rawPayload_;
}

auto CanFrame::extractByte(std::size_t byteIndex) const -> std::uint8_t {
  if (byteIndex >= 8) {
    throw std::out_of_range("CAN frame byte index must be in the range 0-7");
  }
  constexpr std::size_t mostSignificantByteShift{56};
  const auto shiftAmount = mostSignificantByteShift - (byteIndex * 8);
  return static_cast<std::uint8_t>((rawPayload_ >> shiftAmount) & 0xFFULL);
}

auto CanFrame::extractUint16BigEndian(std::size_t startByteIndex) const -> std::uint16_t {
  const auto highByte = static_cast<std::uint16_t>(extractByte(startByteIndex));
  const auto lowByte = static_cast<std::uint16_t>(extractByte(startByteIndex + 1));
  return static_cast<std::uint16_t>((highByte << 8) | lowByte);
}

auto CanFrame::getEngineRpm() const -> std::uint16_t {
  return extractUint16BigEndian(0);
}

auto CanFrame::getVehicleSpeed() const -> std::uint8_t {
  return extractByte(2);
}

auto CanFrame::getSteeringAngle() const -> std::uint16_t {
  return extractUint16BigEndian(3);
}

}  // namespace sdv
