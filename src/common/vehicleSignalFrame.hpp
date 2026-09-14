#pragma once

#include "canFrame.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace sdv {

struct VehicleSignalFrame {
  std::uint32_t sequenceNumber{};
  std::uint16_t engineRpm{};
  std::uint8_t vehicleSpeed{};
  std::uint16_t steeringAngle{};
  std::uint64_t sourceCanPayload{};
  std::uint64_t timestampMs{};
};

inline constexpr std::size_t vehicleSignalFrameByteSize{28};

[[nodiscard]] auto makeVehicleSignalFrame(
    std::uint32_t sequenceNumber,
    const CanFrame& canFrame,
    std::uint64_t timestampMs) -> VehicleSignalFrame;

[[nodiscard]] auto serialiseVehicleSignalFrame(const VehicleSignalFrame& frame)
    -> std::vector<std::uint8_t>;

[[nodiscard]] auto deserialiseVehicleSignalFrame(std::span<const std::uint8_t> bytes)
    -> std::optional<VehicleSignalFrame>;

[[nodiscard]] auto isPlausibleVehicleSignal(const VehicleSignalFrame& frame) -> bool;

[[nodiscard]] auto processVehicleSignalPayload(std::span<const std::uint8_t> bytes)
    -> std::optional<VehicleSignalFrame>;

[[nodiscard]] auto operator==(const VehicleSignalFrame& left, const VehicleSignalFrame& right)
    -> bool;

}  // namespace sdv
