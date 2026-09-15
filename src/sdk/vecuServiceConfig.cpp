#include "vecuServiceConfig.hpp"

#include "../common/platformPaths.hpp"
#include "../common/serviceIds.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace vecu {
namespace {

auto trim(std::string value) -> std::string {
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
    value.erase(value.begin());
  }
  while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
    value.pop_back();
  }
  return value;
}

auto stripQuotes(std::string value) -> std::string {
  value = trim(value);
  if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
    return value.substr(1, value.size() - 2);
  }
  return value;
}

auto parseU16(const std::string& raw) -> std::optional<std::uint16_t> {
  const auto value = stripQuotes(raw);
  if (value.empty()) {
    return std::nullopt;
  }
  try {
    const auto parsed = std::stoul(value, nullptr, 0);
    if (parsed > 0xFFFFUL) {
      return std::nullopt;
    }
    return static_cast<std::uint16_t>(parsed);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

auto findMatchingBrace(const std::string& text, std::size_t openIndex) -> std::size_t {
  int depth = 0;
  for (std::size_t index = openIndex; index < text.size(); ++index) {
    if (text[index] == '{') {
      ++depth;
    } else if (text[index] == '}') {
      --depth;
      if (depth == 0) {
        return index;
      }
    }
  }
  return std::string::npos;
}

auto extractObject(const std::string& text, const std::string& key)
    -> std::optional<std::string> {
  const auto needle = "\"" + key + "\"";
  const auto keyPos = text.find(needle);
  if (keyPos == std::string::npos) {
    return std::nullopt;
  }
  const auto colon = text.find(':', keyPos + needle.size());
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  const auto open = text.find('{', colon);
  if (open == std::string::npos) {
    return std::nullopt;
  }
  const auto close = findMatchingBrace(text, open);
  if (close == std::string::npos) {
    return std::nullopt;
  }
  return text.substr(open, close - open + 1);
}

auto extractString(const std::string& text, const std::string& key) -> std::optional<std::string> {
  const auto needle = "\"" + key + "\"";
  const auto keyPos = text.find(needle);
  if (keyPos == std::string::npos) {
    return std::nullopt;
  }
  const auto colon = text.find(':', keyPos + needle.size());
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  const auto firstQuote = text.find('"', colon + 1);
  if (firstQuote == std::string::npos) {
    return std::nullopt;
  }
  const auto secondQuote = text.find('"', firstQuote + 1);
  if (secondQuote == std::string::npos) {
    return std::nullopt;
  }
  return text.substr(firstQuote + 1, secondQuote - firstQuote - 1);
}

auto extractU16(const std::string& text, const std::string& key) -> std::optional<std::uint16_t> {
  const auto needle = "\"" + key + "\"";
  const auto keyPos = text.find(needle);
  if (keyPos == std::string::npos) {
    return std::nullopt;
  }
  const auto colon = text.find(':', keyPos + needle.size());
  if (colon == std::string::npos) {
    return std::nullopt;
  }
  std::size_t end = colon + 1;
  while (end < text.size() && std::isspace(static_cast<unsigned char>(text[end])) != 0) {
    ++end;
  }
  if (end < text.size() && text[end] == '"') {
    const auto close = text.find('"', end + 1);
    if (close == std::string::npos) {
      return std::nullopt;
    }
    return parseU16(text.substr(end, close - end + 1));
  }
  const auto begin = end;
  while (end < text.size()
         && (std::isalnum(static_cast<unsigned char>(text[end])) != 0 || text[end] == 'x'
             || text[end] == 'X')) {
    ++end;
  }
  return parseU16(text.substr(begin, end - begin));
}

auto extractMethodIds(const std::string& serviceObject) -> std::vector<std::uint16_t> {
  std::vector<std::uint16_t> methods;
  const auto keyPos = serviceObject.find("\"methodIds\"");
  if (keyPos == std::string::npos) {
    return methods;
  }
  const auto open = serviceObject.find('[', keyPos);
  const auto close = serviceObject.find(']', open);
  if (open == std::string::npos || close == std::string::npos) {
    return methods;
  }
  const auto body = serviceObject.substr(open + 1, close - open - 1);
  std::stringstream stream{body};
  std::string token;
  while (std::getline(stream, token, ',')) {
    if (const auto value = parseU16(token); value.has_value()) {
      methods.push_back(*value);
    }
  }
  return methods;
}

}  // namespace

auto loadServiceConfig(const std::string& path) -> std::optional<VecuServiceConfig> {
  const auto resolvedPath = sdv::resolveConfigPath(path);
  std::ifstream input{resolvedPath};
  if (!input) {
    return std::nullopt;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const auto text = buffer.str();

  const auto serviceObject = extractObject(text, "service");
  if (!serviceObject.has_value()) {
    return std::nullopt;
  }

  VecuServiceConfig config;
  config.applicationName = extractString(text, "applicationName").value_or("");
  config.vsomeipConfiguration = extractString(text, "vsomeipConfiguration").value_or("");
  if (!config.vsomeipConfiguration.empty()) {
    config.vsomeipConfiguration = sdv::resolveConfigPath(config.vsomeipConfiguration);
  }
  config.serviceId = extractU16(*serviceObject, "serviceId").value_or(0);
  config.instanceId = extractU16(*serviceObject, "instanceId").value_or(0);
  config.eventId = extractU16(*serviceObject, "eventId").value_or(0);
  config.eventGroupId = extractU16(*serviceObject, "eventGroupId").value_or(0);
  config.methodIds = extractMethodIds(*serviceObject);

  if (config.applicationName.empty() || config.serviceId == 0 || config.instanceId == 0
      || config.eventId == 0 || config.eventGroupId == 0) {
    return std::nullopt;
  }
  return config;
}

auto resolveServiceConfigPath(int argc, char** argv, const char* defaultPath) -> std::string {
  for (int index = 1; index + 1 < argc; ++index) {
    const std::string argument{argv[index]};
    if (argument == "--service-config" || argument == "--config") {
      return sdv::resolveConfigPath(argv[index + 1]);
    }
  }
  if (const auto fromEnv = sdv::getEnvironmentVariable("VECU_SERVICE_CONFIG"); !fromEnv.empty()) {
    return sdv::resolveConfigPath(fromEnv);
  }
  return sdv::resolveConfigPath(defaultPath);
}

auto defaultSensorPublisherConfig() -> VecuServiceConfig {
  return VecuServiceConfig{
      .applicationName = sdv::ids::publisherApplicationName,
      .serviceId = sdv::ids::sensorServiceId,
      .instanceId = sdv::ids::sensorInstanceId,
      .eventId = sdv::ids::sensorEventId,
      .eventGroupId = sdv::ids::sensorEventGroupId,
      .methodIds =
          {
              sdv::ids::setIntervalMethodId,
              sdv::ids::resetSequenceMethodId,
              sdv::ids::setStreamingMethodId,
          },
      .vsomeipConfiguration = sdv::resolveConfigPath("config/vsomeipLocal.json"),
  };
}

auto defaultSensorSubscriberConfig() -> VecuServiceConfig {
  auto config = defaultSensorPublisherConfig();
  config.applicationName = sdv::ids::subscriberApplicationName;
  return config;
}

auto defaultCanGatewayConfig() -> VecuServiceConfig {
  return VecuServiceConfig{
      .applicationName = "canGateway",
      .serviceId = 0x2345,
      .instanceId = 0x6789,
      .eventId = 0x9001,
      .eventGroupId = 0x5566,
      .methodIds = {},
      .vsomeipConfiguration = sdv::resolveConfigPath("config/vsomeipCanBridge.json"),
  };
}

auto defaultCanSubscriberConfig() -> VecuServiceConfig {
  auto config = defaultCanGatewayConfig();
  config.applicationName = "canSubscriber";
  return config;
}

}  // namespace vecu
