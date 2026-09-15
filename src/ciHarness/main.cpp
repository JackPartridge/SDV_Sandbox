#include "vecu_sdk.hpp"

#include "platformPaths.hpp"
#include "telemetryFrame.hpp"
#include "vehicleSignalFrame.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct ContractSpec {
  std::string payloadKind{"telemetry"};
  std::size_t minFrames{5};
  std::chrono::seconds timeout{15};
  bool requireMonotonicSequence{true};
};

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

auto extractBool(const std::string& text, const std::string& key, bool fallback) -> bool {
  const auto needle = "\"" + key + "\"";
  const auto keyPos = text.find(needle);
  if (keyPos == std::string::npos) {
    return fallback;
  }
  const auto colon = text.find(':', keyPos + needle.size());
  if (colon == std::string::npos) {
    return fallback;
  }
  const auto truePos = text.find("true", colon);
  const auto falsePos = text.find("false", colon);
  if (truePos != std::string::npos
      && (falsePos == std::string::npos || truePos < falsePos)) {
    return true;
  }
  if (falsePos != std::string::npos) {
    return false;
  }
  return fallback;
}

auto extractU64(const std::string& text, const std::string& key, std::uint64_t fallback)
    -> std::uint64_t {
  const auto needle = "\"" + key + "\"";
  const auto keyPos = text.find(needle);
  if (keyPos == std::string::npos) {
    return fallback;
  }
  const auto colon = text.find(':', keyPos + needle.size());
  if (colon == std::string::npos) {
    return fallback;
  }
  try {
    return std::stoull(text.substr(colon + 1));
  } catch (const std::exception&) {
    return fallback;
  }
}

auto loadContract(const std::string& path) -> std::optional<ContractSpec> {
  std::ifstream input{path};
  if (!input) {
    return std::nullopt;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const auto text = buffer.str();

  ContractSpec contract;
  contract.payloadKind = extractString(text, "payloadKind").value_or("telemetry");
  contract.minFrames = static_cast<std::size_t>(extractU64(text, "minFrames", 5));
  contract.timeout = std::chrono::seconds{extractU64(text, "timeoutSeconds", 15)};
  contract.requireMonotonicSequence = extractBool(text, "requireMonotonicSequence", true);
  return contract;
}

auto resolveArgValue(int argc, char** argv, const std::string& flag, const char* fallback)
    -> std::string {
  for (int index = 1; index + 1 < argc; ++index) {
    if (flag == argv[index]) {
      return argv[index + 1];
    }
  }
  return fallback;
}

struct HarnessState {
  std::mutex mutex;
  std::size_t accepted{0};
  std::size_t rejected{0};
  std::optional<std::uint32_t> lastSequence;
  bool sequenceFault{false};
  std::string lastError;
};

auto validatePayload(
    const ContractSpec& contract,
    const std::vector<std::uint8_t>& payload,
    HarnessState& state) -> bool {
  std::optional<std::uint32_t> sequence;

  if (contract.payloadKind == "vehicleSignal") {
    const auto frame = sdv::processVehicleSignalPayload(payload);
    if (!frame.has_value()) {
      state.lastError = "vehicle signal contract violation";
      return false;
    }
    sequence = frame->sequenceNumber;
  } else {
    const auto frame = sdv::processTelemetryPayload(payload);
    if (!frame.has_value()) {
      state.lastError = "telemetry contract violation";
      return false;
    }
    sequence = frame->sequenceNumber;
  }

  if (contract.requireMonotonicSequence && state.lastSequence.has_value()) {
    if (*sequence != *state.lastSequence + 1) {
      state.sequenceFault = true;
      state.lastError = "non-monotonic sequence";
      return false;
    }
  }
  state.lastSequence = sequence;
  return true;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  const bool verbose = vecu::wantsVerboseLoggingFromArgs(argc, argv);
  const auto serviceConfigPath = sdv::resolveConfigPath(resolveArgValue(
      argc,
      argv,
      "--service-config",
      "config/services/sensorSubscriber.json"));
  const auto contractPath = sdv::resolveConfigPath(resolveArgValue(
      argc,
      argv,
      "--contract",
      "config/contracts/telemetryContract.json"));

  auto serviceConfig = vecu::loadServiceConfig(serviceConfigPath);
  if (!serviceConfig.has_value()) {
    std::cerr << "ciContractHarness: failed to load service config: " << serviceConfigPath << '\n';
    return EXIT_FAILURE;
  }

  const auto contract = loadContract(contractPath);
  if (!contract.has_value()) {
    std::cerr << "ciContractHarness: failed to load contract: " << contractPath << '\n';
    return EXIT_FAILURE;
  }

  HarnessState state;
  vecu::VecuSubscriber subscriber;
  const auto started = subscriber.start(
      *serviceConfig,
      [&](const std::vector<std::uint8_t>& payload) {
        std::scoped_lock lock{state.mutex};
        if (validatePayload(*contract, payload, state)) {
          ++state.accepted;
          if (verbose) {
            std::cout << "ciContractHarness: accepted frame #" << state.accepted
                      << " (" << payload.size() << " bytes)\n";
          }
        } else {
          ++state.rejected;
          std::cerr << "ciContractHarness: rejected frame — " << state.lastError << '\n';
        }
      },
      verbose);

  if (!started) {
    std::cerr << "ciContractHarness: failed to start subscriber\n";
    return EXIT_FAILURE;
  }

  std::cout << "ciContractHarness: listening for " << contract->minFrames
            << " valid '" << contract->payloadKind << "' frames "
            << "(timeout " << contract->timeout.count() << "s)\n";

  const auto deadline = std::chrono::steady_clock::now() + contract->timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    {
      std::scoped_lock lock{state.mutex};
      if (state.rejected > 0 || state.sequenceFault) {
        subscriber.stop();
        std::cerr << "ciContractHarness: FAIL\n";
        return EXIT_FAILURE;
      }
      if (state.accepted >= contract->minFrames) {
        subscriber.stop();
        std::cout << "ciContractHarness: PASS (" << state.accepted << " frames)\n";
        return EXIT_SUCCESS;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
  }

  subscriber.stop();
  std::cerr << "ciContractHarness: FAIL (timeout, accepted=" << state.accepted
            << ", rejected=" << state.rejected << ")\n";
  return EXIT_FAILURE;
}
