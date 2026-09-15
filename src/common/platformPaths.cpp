#include "platformPaths.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#if defined(_WIN32)
#  include <stdlib.h>
#endif

namespace sdv {
namespace {

auto pathExists(const std::filesystem::path& path) -> bool {
  std::error_code error;
  return std::filesystem::exists(path, error);
}

}  // namespace

auto setEnvironmentVariable(const std::string& name, const std::string& value) -> void {
#if defined(_WIN32)
  _putenv_s(name.c_str(), value.c_str());
#else
  setenv(name.c_str(), value.c_str(), 1);
#endif
}

auto getEnvironmentVariable(const std::string& name) -> std::string {
  if (const char* value = std::getenv(name.c_str()); value != nullptr) {
    return value;
  }
  return {};
}

auto resolveConfigPath(const std::string& pathOrRelative) -> std::string {
  namespace fs = std::filesystem;
  const fs::path input{pathOrRelative};
  if (input.is_absolute() && pathExists(input)) {
    return input.string();
  }

  if (const auto root = getEnvironmentVariable("VECU_CONFIG_ROOT"); !root.empty()) {
    const auto candidate = fs::path{root} / input;
    if (pathExists(candidate)) {
      return fs::weakly_canonical(candidate).string();
    }
  }

  if (pathExists(input)) {
    return fs::weakly_canonical(input).string();
  }

  const auto fromCwd = fs::current_path() / input;
  if (pathExists(fromCwd)) {
    return fs::weakly_canonical(fromCwd).string();
  }

#if !defined(_WIN32)
  const fs::path dockerRoot{"/workspace"};
  const auto fromDocker = dockerRoot / input;
  if (pathExists(fromDocker)) {
    return fromDocker.string();
  }
  if (input.is_absolute() && pathExists(input)) {
    return input.string();
  }
#endif

  return pathOrRelative;
}

auto defaultVsomeipConfigPath(bool verboseLogging) -> std::string {
  return resolveConfigPath(
      verboseLogging ? "config/vsomeipVerbose.json" : "config/vsomeipLocal.json");
}

}  // namespace sdv
