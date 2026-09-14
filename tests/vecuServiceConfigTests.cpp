#include <gtest/gtest.h>

#include "vecuServiceConfig.hpp"

#include <filesystem>
#include <fstream>

namespace {

auto writeTempConfig(const std::filesystem::path& path) -> void {
  std::ofstream output{path};
  output << R"({
  "applicationName": "unitTestPublisher",
  "vsomeipConfiguration": "/tmp/vsomeip.json",
  "service": {
    "serviceId": "0xABCD",
    "instanceId": "0x10",
    "eventId": "0x20",
    "eventGroupId": "0x30",
    "methodIds": ["0x1", "0x2", "0x3"]
  }
})";
}

TEST(VecuServiceConfig, LoadsJsonWithoutRecompile) {
  const auto path = std::filesystem::temp_directory_path() / "vecuServiceConfigTest.json";
  writeTempConfig(path);

  const auto config = vecu::loadServiceConfig(path.string());
  ASSERT_TRUE(config.has_value());
  EXPECT_EQ(config->applicationName, "unitTestPublisher");
  EXPECT_EQ(config->serviceId, 0xABCD);
  EXPECT_EQ(config->instanceId, 0x10);
  EXPECT_EQ(config->eventId, 0x20);
  EXPECT_EQ(config->eventGroupId, 0x30);
  ASSERT_EQ(config->methodIds.size(), 3U);
  EXPECT_EQ(config->methodIds[0], 0x1);
  EXPECT_EQ(config->vsomeipConfiguration, "/tmp/vsomeip.json");
}

}  // namespace
