#pragma once

#include "../common/hardwareCollector.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vsomeip/vsomeip.hpp>

namespace sdv {

class SensorPublisher {
public:
  explicit SensorPublisher(
      std::chrono::milliseconds publishInterval = std::chrono::milliseconds{500});

  [[nodiscard]] auto init() -> bool;
  auto start() -> void;
  auto stop() -> void;

private:
  auto onState(vsomeip::state_type_e state) -> void;
  auto onMessage(const std::shared_ptr<vsomeip::message>& message) -> void;
  auto publishLoop(std::stop_token stopToken) -> void;
  auto sendMethodResponse(
      const std::shared_ptr<vsomeip::message>& request,
      const std::vector<vsomeip::byte_t>& payloadBytes) -> void;

  std::shared_ptr<vsomeip::application> application_;
  std::shared_ptr<vsomeip::payload> payload_;
  HardwareCollector collector_;
  std::atomic<std::uint32_t> sequence_{0};
  std::atomic<std::int64_t> publishIntervalMs_{500};
  std::atomic<bool> streamingEnabled_{true};
  std::jthread publishThread_;
  std::atomic<bool> offered_{false};
};

}  // namespace sdv
