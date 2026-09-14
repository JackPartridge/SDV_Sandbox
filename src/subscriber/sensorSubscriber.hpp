#pragma once

#include "../common/telemetryFrame.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vsomeip/vsomeip.hpp>

namespace sdv {

class SensorSubscriber {
public:
  using FrameHandler = std::function<void(const TelemetryFrame&)>;

  explicit SensorSubscriber(FrameHandler onFrame = nullptr);

  [[nodiscard]] auto init() -> bool;
  auto start() -> void;
  auto stop() -> void;

  [[nodiscard]] auto receivedCount() const -> std::size_t;
  [[nodiscard]] auto lastFrame() const -> std::optional<TelemetryFrame>;
  [[nodiscard]] auto rejectedCount() const -> std::size_t;

private:
  auto onState(vsomeip::state_type_e state) -> void;
  auto onAvailability(vsomeip::service_t service, vsomeip::instance_t instance, bool isAvailable)
      -> void;
  auto onMessage(const std::shared_ptr<vsomeip::message>& message) -> void;

  std::shared_ptr<vsomeip::application> application_;
  FrameHandler onFrame_;
  mutable std::mutex framesMutex_;
  std::optional<TelemetryFrame> lastFrame_;
  std::atomic<std::size_t> receivedCount_{0};
  std::atomic<std::size_t> rejectedCount_{0};
};

}  // namespace sdv
