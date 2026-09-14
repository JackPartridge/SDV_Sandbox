#pragma once

#include <cstdint>

namespace sdv::ids {

inline constexpr std::uint16_t sensorServiceId{0x1234};
inline constexpr std::uint16_t sensorInstanceId{0x5678};
inline constexpr std::uint16_t sensorEventId{0x8778};
inline constexpr std::uint16_t sensorEventGroupId{0x4465};

inline constexpr std::uint16_t setIntervalMethodId{0x0001};
inline constexpr std::uint16_t resetSequenceMethodId{0x0002};
inline constexpr std::uint16_t setStreamingMethodId{0x0003};

inline constexpr char publisherApplicationName[]{"sensorPublisher"};
inline constexpr char subscriberApplicationName[]{"sensorSubscriber"};

}  // namespace sdv::ids
