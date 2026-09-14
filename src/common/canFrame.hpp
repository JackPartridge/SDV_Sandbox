#pragma once

#include <cstdint>

namespace sdv {

class CanFrame {
public:
  explicit CanFrame(std::uint64_t rawPayload);

  [[nodiscard]] auto getEngineRpm() const -> std::uint16_t;
  [[nodiscard]] auto getVehicleSpeed() const -> std::uint8_t;
  [[nodiscard]] auto getSteeringAngle() const -> std::uint16_t;
  [[nodiscard]] auto getRawPayload() const -> std::uint64_t;

private:
  [[nodiscard]] auto extractByte(std::size_t byteIndex) const -> std::uint8_t;
  [[nodiscard]] auto extractUint16BigEndian(std::size_t startByteIndex) const -> std::uint16_t;

  std::uint64_t rawPayload_{0};
};

}  // namespace sdv
