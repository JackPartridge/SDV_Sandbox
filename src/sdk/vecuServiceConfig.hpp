#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#if defined(_WIN32)
#  if defined(VECU_SDK_BUILD)
#    define VECU_SDK_API __declspec(dllexport)
#  else
#    define VECU_SDK_API __declspec(dllimport)
#  endif
#else
#  if defined(VECU_SDK_BUILD)
#    define VECU_SDK_API __attribute__((visibility("default")))
#  else
#    define VECU_SDK_API
#  endif
#endif

namespace vecu {

struct VecuServiceConfig {
  std::string applicationName;
  std::uint16_t serviceId{};
  std::uint16_t instanceId{};
  std::uint16_t eventId{};
  std::uint16_t eventGroupId{};
  std::vector<std::uint16_t> methodIds;
  std::string vsomeipConfiguration;
};

[[nodiscard]] VECU_SDK_API auto loadServiceConfig(const std::string& path)
    -> std::optional<VecuServiceConfig>;

[[nodiscard]] VECU_SDK_API auto resolveServiceConfigPath(
    int argc,
    char** argv,
    const char* defaultPath) -> std::string;

[[nodiscard]] VECU_SDK_API auto defaultSensorPublisherConfig() -> VecuServiceConfig;
[[nodiscard]] VECU_SDK_API auto defaultSensorSubscriberConfig() -> VecuServiceConfig;
[[nodiscard]] VECU_SDK_API auto defaultCanGatewayConfig() -> VecuServiceConfig;
[[nodiscard]] VECU_SDK_API auto defaultCanSubscriberConfig() -> VecuServiceConfig;

}  // namespace vecu
