#include "vecu_sdk.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <type_traits>
#include <vector>

namespace {

auto buildMockHardwareSample(std::uint32_t tick) -> std::vector<std::uint8_t> {
  const std::uint32_t sequenceNumber = tick;
  const std::int32_t temperatureMilliCelsius = 20'000 + static_cast<std::int32_t>(tick) * 250;
  const std::uint64_t timestampMs = 1'700'000'000'000ULL + static_cast<std::uint64_t>(tick) * 1'000ULL;

  std::vector<std::uint8_t> bytes;
  bytes.reserve(16);

  const auto appendLittleEndian = [&bytes](auto value) {
    using Unsigned = std::make_unsigned_t<decltype(value)>;
    auto bits = static_cast<Unsigned>(value);
    for (std::size_t index = 0; index < sizeof(value); ++index) {
      bytes.push_back(static_cast<std::uint8_t>((bits >> (8 * index)) & 0xFFU));
    }
  };

  appendLittleEndian(sequenceNumber);
  appendLittleEndian(temperatureMilliCelsius);
  appendLittleEndian(timestampMs);
  return bytes;
}

}  // namespace

auto main() -> int {
  vecu::VecuPublisher publisher;

  if (!publisher.start()) {
    std::cerr << "example_fetcher: failed to start VecuPublisher SDK\n";
    return 1;
  }

  std::cout << "example_fetcher: hardware fetcher using vecu_sdk (no vsomeip headers required)\n";

  for (std::uint32_t tick = 0; tick < 10; ++tick) {
    const auto sample = buildMockHardwareSample(tick);
    publisher.publish(sample);
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
  }

  publisher.stop();
  std::cout << "example_fetcher: done\n";
  return 0;
}
