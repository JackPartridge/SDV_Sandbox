#pragma once

#include "vecuServiceConfig.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace vecu {

class VECU_SDK_API VecuPublisher {
public:
  VecuPublisher();
  ~VecuPublisher();

  VecuPublisher(const VecuPublisher&) = delete;
  auto operator=(const VecuPublisher&) -> VecuPublisher& = delete;
  VecuPublisher(VecuPublisher&&) noexcept;
  auto operator=(VecuPublisher&&) noexcept -> VecuPublisher&;

  [[nodiscard]] auto start(const VecuServiceConfig& config, bool verboseLogging = false) -> bool;
  [[nodiscard]] auto start(bool verboseLogging = false) -> bool;
  auto stop() -> void;
  auto publish(const std::vector<std::uint8_t>& payload) -> void;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class VECU_SDK_API VecuSubscriber {
public:
  using MessageHandler = std::function<void(const std::vector<std::uint8_t>& payload)>;

  VecuSubscriber();
  ~VecuSubscriber();

  VecuSubscriber(const VecuSubscriber&) = delete;
  auto operator=(const VecuSubscriber&) -> VecuSubscriber& = delete;
  VecuSubscriber(VecuSubscriber&&) noexcept;
  auto operator=(VecuSubscriber&&) noexcept -> VecuSubscriber&;

  [[nodiscard]] auto start(
      const VecuServiceConfig& config,
      MessageHandler onMessage,
      bool verboseLogging = false) -> bool;
  [[nodiscard]] auto start(MessageHandler onMessage, bool verboseLogging = false) -> bool;
  auto stop() -> void;

  auto requestSetIntervalMs(std::uint32_t intervalMs) -> void;
  auto requestResetSequence() -> void;
  auto requestSetStreaming(bool enabled) -> void;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

[[nodiscard]] VECU_SDK_API auto wantsVerboseLoggingFromArgs(int argc, char** argv) -> bool;

}  // namespace vecu
