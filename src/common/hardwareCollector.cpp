#include "hardwareCollector.hpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace sdv {
namespace {

auto readMeminfoKib(const std::string& key) -> std::uint64_t {
  std::ifstream input{"/proc/meminfo"};
  std::string label;
  std::uint64_t kib{0};
  std::string unit;
  while (input >> label >> kib >> unit) {
    if (label == key) {
      return kib;
    }
  }
  return 0;
}

auto nowTimestampMs() -> std::uint64_t {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

auto parseMilliCelsius(std::int64_t raw) -> std::int32_t {
  if (raw > 1000) {
    return static_cast<std::int32_t>(raw);
  }
  return static_cast<std::int32_t>(raw * 1000);
}

auto readTemperatureFromThermalZones() -> std::int32_t {
  namespace fs = std::filesystem;
  const fs::path thermalRoot{"/sys/class/thermal"};
  if (!fs::exists(thermalRoot)) {
    return temperatureUnavailable;
  }

  std::int32_t fallback = temperatureUnavailable;
  for (const auto& entry : fs::directory_iterator{thermalRoot}) {
    if (!entry.is_directory() && !entry.is_symlink()) {
      continue;
    }
    const auto path = entry.path();
    if (path.filename().string().find("thermal_zone") == std::string::npos) {
      continue;
    }

    const auto typePath = path / "type";
    const auto tempPath = path / "temp";
    if (!fs::exists(tempPath)) {
      continue;
    }

    std::ifstream tempFile{tempPath};
    std::int64_t raw{0};
    tempFile >> raw;
    if (!tempFile) {
      continue;
    }

    const auto milliC = parseMilliCelsius(raw);
    std::string type;
    if (fs::exists(typePath)) {
      std::ifstream typeFile{typePath};
      typeFile >> type;
    }

    const bool preferred = type.find("cpu") != std::string::npos
        || type.find("x86") != std::string::npos
        || type.find("pkg") != std::string::npos
        || type.find("k10") != std::string::npos
        || type == "acpitz";
    if (preferred) {
      return milliC;
    }
    if (fallback == temperatureUnavailable) {
      fallback = milliC;
    }
  }
  return fallback;
}

auto readTemperatureFromHwmon() -> std::int32_t {
  namespace fs = std::filesystem;
  const fs::path hwmonRoot{"/sys/class/hwmon"};
  if (!fs::exists(hwmonRoot)) {
    return temperatureUnavailable;
  }

  std::int32_t fallback = temperatureUnavailable;
  for (const auto& entry : fs::directory_iterator{hwmonRoot}) {
    if (!entry.is_directory() && !entry.is_symlink()) {
      continue;
    }
    const auto path = entry.path();
    std::string name;
    if (const auto namePath = path / "name"; fs::exists(namePath)) {
      std::ifstream nameFile{namePath};
      nameFile >> name;
    }

    for (const auto& file : fs::directory_iterator{path}) {
      const auto filename = file.path().filename().string();
      if (filename.rfind("temp", 0) != 0 || filename.find("_input") == std::string::npos) {
        continue;
      }
      std::ifstream tempFile{file.path()};
      std::int64_t raw{0};
      tempFile >> raw;
      if (!tempFile) {
        continue;
      }
      const auto milliC = parseMilliCelsius(raw);
      const bool preferred = name.find("k10") != std::string::npos
          || name.find("coretemp") != std::string::npos
          || name.find("cpu") != std::string::npos
          || name.find("zenpower") != std::string::npos;
      if (preferred) {
        return milliC;
      }
      if (fallback == temperatureUnavailable) {
        fallback = milliC;
      }
    }
  }
  return fallback;
}

}  // namespace

auto HardwareCollector::readCpuUtilisationMilliPercent() -> std::int32_t {
  std::ifstream input{"/proc/stat"};
  std::string cpuLabel;
  std::uint64_t user{0};
  std::uint64_t nice{0};
  std::uint64_t system{0};
  std::uint64_t idle{0};
  std::uint64_t ioWait{0};
  std::uint64_t irq{0};
  std::uint64_t softIrq{0};
  std::uint64_t steal{0};

  if (!(input >> cpuLabel >> user >> nice >> system >> idle >> ioWait >> irq >> softIrq >> steal)) {
    return 0;
  }

  const auto idleAll = idle + ioWait;
  const auto total = user + nice + system + idleAll + irq + softIrq + steal;

  if (!hasPreviousCpuSample_) {
    previousIdle_ = idleAll;
    previousTotal_ = total;
    hasPreviousCpuSample_ = true;
    return 0;
  }

  const auto idleDelta = idleAll - previousIdle_;
  const auto totalDelta = total - previousTotal_;
  previousIdle_ = idleAll;
  previousTotal_ = total;

  if (totalDelta == 0) {
    return 0;
  }

  const auto busy = totalDelta - idleDelta;
  return static_cast<std::int32_t>((busy * 100'000ULL) / totalDelta);
}

auto HardwareCollector::readMemoryUsedMilliPercent() -> std::int32_t {
  const auto total = readMeminfoKib("MemTotal:");
  const auto available = readMeminfoKib("MemAvailable:");
  if (total == 0 || available > total) {
    return 0;
  }
  const auto used = total - available;
  return static_cast<std::int32_t>((used * 100'000ULL) / total);
}

auto HardwareCollector::readCpuTemperatureMilliCelsius() -> std::int32_t {
  const auto fromThermal = readTemperatureFromThermalZones();
  if (fromThermal != temperatureUnavailable) {
    return fromThermal;
  }
  return readTemperatureFromHwmon();
}

auto HardwareCollector::readLoadAverage1Milli() -> std::int32_t {
  std::ifstream input{"/proc/loadavg"};
  double load1{0.0};
  if (!(input >> load1)) {
    return 0;
  }
  return static_cast<std::int32_t>(std::lround(load1 * 1000.0));
}

auto HardwareCollector::readSwapUsedMilliPercent() -> std::int32_t {
  const auto total = readMeminfoKib("SwapTotal:");
  const auto free = readMeminfoKib("SwapFree:");
  if (total == 0) {
    return 0;
  }
  if (free > total) {
    return 0;
  }
  const auto used = total - free;
  return static_cast<std::int32_t>((used * 100'000ULL) / total);
}

auto HardwareCollector::readProcessCount() -> std::uint32_t {
  std::ifstream input{"/proc/loadavg"};
  double load1{0.0};
  double load5{0.0};
  double load15{0.0};
  std::string runnable;
  std::uint32_t lastPid{0};
  if (!(input >> load1 >> load5 >> load15 >> runnable >> lastPid)) {
    return 0;
  }
  const auto slash = runnable.find('/');
  if (slash == std::string::npos || slash + 1 >= runnable.size()) {
    return 0;
  }
  try {
    return static_cast<std::uint32_t>(std::stoul(runnable.substr(slash + 1)));
  } catch (const std::exception&) {
    return 0;
  }
}

auto HardwareCollector::readNetworkTotals(std::uint64_t& rxBytes, std::uint64_t& txBytes) -> void {
  rxBytes = 0;
  txBytes = 0;
  std::ifstream input{"/proc/net/dev"};
  std::string line;
  std::getline(input, line);
  std::getline(input, line);
  while (std::getline(input, line)) {
    const auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    auto iface = line.substr(0, colon);
    while (!iface.empty() && iface.front() == ' ') {
      iface.erase(iface.begin());
    }
    while (!iface.empty() && iface.back() == ' ') {
      iface.pop_back();
    }
    if (iface == "lo") {
      continue;
    }
    std::istringstream values{line.substr(colon + 1)};
    std::uint64_t rx{0};
    std::uint64_t tx{0};
    std::uint64_t ignored{0};
    values >> rx;
    for (int index = 0; index < 7; ++index) {
      values >> ignored;
    }
    values >> tx;
    rxBytes += rx;
    txBytes += tx;
  }
}

auto HardwareCollector::readNetworkRates(
    std::uint32_t& rxBytesPerSec,
    std::uint32_t& txBytesPerSec) -> void {
  std::uint64_t rx{0};
  std::uint64_t tx{0};
  readNetworkTotals(rx, tx);
  const auto now = std::chrono::steady_clock::now();

  if (!hasPreviousNetworkSample_) {
    previousRxBytes_ = rx;
    previousTxBytes_ = tx;
    previousNetworkSampleAt_ = now;
    hasPreviousNetworkSample_ = true;
    rxBytesPerSec = 0;
    txBytesPerSec = 0;
    return;
  }

  const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                             now - previousNetworkSampleAt_)
                             .count();
  if (elapsedMs <= 0) {
    rxBytesPerSec = 0;
    txBytesPerSec = 0;
    return;
  }

  const auto rxDelta = rx >= previousRxBytes_ ? rx - previousRxBytes_ : 0;
  const auto txDelta = tx >= previousTxBytes_ ? tx - previousTxBytes_ : 0;
  previousRxBytes_ = rx;
  previousTxBytes_ = tx;
  previousNetworkSampleAt_ = now;

  rxBytesPerSec = static_cast<std::uint32_t>((rxDelta * 1000ULL) / static_cast<std::uint64_t>(elapsedMs));
  txBytesPerSec = static_cast<std::uint32_t>((txDelta * 1000ULL) / static_cast<std::uint64_t>(elapsedMs));
}

auto HardwareCollector::readLogicalCpuCount() -> std::uint32_t {
  const auto count = std::thread::hardware_concurrency();
  return count == 0 ? 1U : count;
}

auto HardwareCollector::sample(std::uint32_t sequenceNumber) -> TelemetryFrame {
  std::uint32_t rxRate{0};
  std::uint32_t txRate{0};
  readNetworkRates(rxRate, txRate);

  return TelemetryFrame{
      .sequenceNumber = sequenceNumber,
      .cpuUtilisationMilliPercent = readCpuUtilisationMilliPercent(),
      .memoryUsedMilliPercent = readMemoryUsedMilliPercent(),
      .temperatureMilliCelsius = readCpuTemperatureMilliCelsius(),
      .timestampMs = nowTimestampMs(),
      .loadAverage1Milli = readLoadAverage1Milli(),
      .swapUsedMilliPercent = readSwapUsedMilliPercent(),
      .processCount = readProcessCount(),
      .networkRxBytesPerSec = rxRate,
      .networkTxBytesPerSec = txRate,
      .logicalCpuCount = readLogicalCpuCount(),
  };
}

}  // namespace sdv
