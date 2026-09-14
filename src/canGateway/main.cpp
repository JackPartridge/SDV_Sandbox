#include "vecu_sdk.hpp"

#include "canBusSimulator.hpp"
#include "consoleLog.hpp"
#include "vehicleSignalFrame.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <thread>

namespace {

auto nowTimestampMs() -> std::uint64_t {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

}  // namespace

auto main(int argc, char** argv) -> int {
  const bool verbose = vecu::wantsVerboseLoggingFromArgs(argc, argv);
  const auto configPath = vecu::resolveServiceConfigPath(
      argc,
      argv,
      "/workspace/config/services/canGateway.json");

  auto config = vecu::loadServiceConfig(configPath);
  if (!config.has_value()) {
    std::cerr << "canGateway: failed to load service config from " << configPath
              << " — falling back to defaults\n";
    config = vecu::defaultCanGatewayConfig();
  }

  vecu::VecuPublisher publisher;
  if (!publisher.start(*config, verbose)) {
    std::cerr << "canGateway: failed to start SOME/IP publisher\n";
    return EXIT_FAILURE;
  }

  sdv::logLine(
      "CAN→SOME/IP gateway online (app=" + config->applicationName
      + ", serviceConfig=" + configPath + ")");

  sdv::CanBusSimulator canBus;
  std::uint32_t sequence{0};
  while (true) {
    const auto canFrame = canBus.nextFrame();
    const auto signal = sdv::makeVehicleSignalFrame(sequence++, canFrame, nowTimestampMs());
    publisher.publish(sdv::serialiseVehicleSignalFrame(signal));

    if (verbose) {
      std::ostringstream line;
      line << "Bridged CAN raw=0x" << std::hex << canFrame.getRawPayload() << std::dec
           << " -> RPM=" << signal.engineRpm
           << " speed=" << static_cast<unsigned>(signal.vehicleSpeed)
           << " steer=" << signal.steeringAngle;
      sdv::logLine(line.str());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{200});
  }
}
