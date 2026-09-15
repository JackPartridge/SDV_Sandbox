#pragma once

#include <string>

namespace sdv {

auto setEnvironmentVariable(const std::string& name, const std::string& value) -> void;

[[nodiscard]] auto getEnvironmentVariable(const std::string& name) -> std::string;

[[nodiscard]] auto resolveConfigPath(const std::string& pathOrRelative) -> std::string;

[[nodiscard]] auto defaultVsomeipConfigPath(bool verboseLogging) -> std::string;

}  // namespace sdv
