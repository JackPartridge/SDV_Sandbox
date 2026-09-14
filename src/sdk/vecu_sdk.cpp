#include "vecu_sdk.hpp"

#include "../common/consoleLog.hpp"
#include "../common/serviceIds.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <vsomeip/vsomeip.hpp>

namespace vecu {
namespace {

constexpr auto offerWaitTimeout = std::chrono::seconds{5};
constexpr char quietConfigPath[]{"/workspace/config/vsomeipLocal.json"};
constexpr char verboseConfigPath[]{"/workspace/config/vsomeipVerbose.json"};

auto envFlagEnabled(const char* name) -> bool {
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return false;
  }
  return std::strcmp(value, "1") == 0
      || std::strcmp(value, "true") == 0
      || std::strcmp(value, "TRUE") == 0
      || std::strcmp(value, "yes") == 0;
}

auto applyVsomeipConfiguration(const VecuServiceConfig& config, bool verboseLogging) -> void {
  if (!config.vsomeipConfiguration.empty()) {
    setenv("VSOMEIP_CONFIGURATION", config.vsomeipConfiguration.c_str(), 1);
    return;
  }
  const char* selected = verboseLogging ? verboseConfigPath : quietConfigPath;
  setenv("VSOMEIP_CONFIGURATION", selected, 1);
}

auto resolveVerbose(bool verboseLogging) -> bool {
  return verboseLogging || envFlagEnabled("VECU_VERBOSE");
}

auto methodOrDefault(const VecuServiceConfig& config, std::size_t index, std::uint16_t fallback)
    -> std::uint16_t {
  if (index < config.methodIds.size()) {
    return config.methodIds[index];
  }
  return fallback;
}

}  // namespace

struct VecuPublisher::Impl {
  VecuServiceConfig config;
  std::shared_ptr<vsomeip::application> application;
  std::shared_ptr<vsomeip::payload> payload;
  std::mutex payloadMutex;
  std::mutex offerMutex;
  std::condition_variable offerCondition;
  std::atomic<bool> offered{false};
  std::atomic<bool> running{false};
  bool verboseLogging{false};
  std::jthread runtimeThread;

  auto onState(vsomeip::state_type_e state) -> void {
    if (state != vsomeip::state_type_e::ST_REGISTERED) {
      return;
    }

    application->offer_service(config.serviceId, config.instanceId);
    offered.store(true);
    offerCondition.notify_all();

    if (verboseLogging) {
      std::ostringstream line;
      line << std::hex << std::setfill('0')
           << "VecuPublisher OFFER service=0x" << std::setw(4) << config.serviceId
           << " instance=0x" << std::setw(4) << config.instanceId
           << " event=0x" << std::setw(4) << config.eventId;
      sdv::logLine(line.str());
    }
  }
};

VecuPublisher::VecuPublisher() : impl_{std::make_unique<Impl>()} {}

VecuPublisher::~VecuPublisher() {
  stop();
}

VecuPublisher::VecuPublisher(VecuPublisher&&) noexcept = default;

auto VecuPublisher::operator=(VecuPublisher&&) noexcept -> VecuPublisher& = default;

auto VecuPublisher::start(const VecuServiceConfig& config, bool verboseLogging) -> bool {
  if (impl_->running.load()) {
    return impl_->offered.load();
  }

  impl_->config = config;
  impl_->verboseLogging = resolveVerbose(verboseLogging);
  applyVsomeipConfiguration(impl_->config, impl_->verboseLogging);

  impl_->application =
      vsomeip::runtime::get()->create_application(impl_->config.applicationName);
  impl_->payload = vsomeip::runtime::get()->create_payload();

  if (!impl_->application->init()) {
    sdv::logLine("VecuPublisher failed to initialise vsomeip application");
    return false;
  }

  impl_->application->register_state_handler(
      [this](vsomeip::state_type_e state) { impl_->onState(state); });

  std::set<vsomeip::eventgroup_t> eventGroups{impl_->config.eventGroupId};
  impl_->application->offer_event(
      impl_->config.serviceId,
      impl_->config.instanceId,
      impl_->config.eventId,
      eventGroups,
      vsomeip::event_type_e::ET_FIELD,
      std::chrono::milliseconds::zero(),
      false,
      true,
      nullptr,
      vsomeip::reliability_type_e::RT_UNRELIABLE);

  impl_->offered.store(false);
  impl_->running.store(true);
  impl_->runtimeThread = std::jthread([this](std::stop_token /*stopToken*/) {
    impl_->application->start();
  });

  std::unique_lock lock{impl_->offerMutex};
  const auto offeredInTime = impl_->offerCondition.wait_for(lock, offerWaitTimeout, [this] {
    return impl_->offered.load();
  });

  if (!offeredInTime) {
    sdv::logLine("VecuPublisher timed out waiting for service offer");
    stop();
    return false;
  }

  if (impl_->verboseLogging) {
    sdv::logLine("VecuPublisher ready to accept publish() calls");
  }
  return true;
}

auto VecuPublisher::start(bool verboseLogging) -> bool {
  return start(defaultSensorPublisherConfig(), verboseLogging);
}

auto VecuPublisher::stop() -> void {
  if (!impl_ || !impl_->running.exchange(false)) {
    return;
  }

  if (impl_->offered.exchange(false) && impl_->application) {
    impl_->application->stop_offer_service(impl_->config.serviceId, impl_->config.instanceId);
  }

  if (impl_->application) {
    impl_->application->clear_all_handler();
    impl_->application->stop();
  }

  if (impl_->runtimeThread.joinable()) {
    impl_->runtimeThread.request_stop();
  }
}

auto VecuPublisher::publish(const std::vector<std::uint8_t>& payload) -> void {
  if (!impl_->offered.load()) {
    sdv::logLine("VecuPublisher publish ignored — service is not offered yet");
    return;
  }

  std::scoped_lock lock{impl_->payloadMutex};
  impl_->payload->set_data(
      payload.data(),
      static_cast<vsomeip::length_t>(payload.size()));
  impl_->application->notify(
      impl_->config.serviceId,
      impl_->config.instanceId,
      impl_->config.eventId,
      impl_->payload);

  if (impl_->verboseLogging) {
    std::ostringstream line;
    line << "VecuPublisher published " << payload.size() << " bytes";
    sdv::logLine(line.str());
  }
}

struct VecuSubscriber::Impl {
  VecuServiceConfig config;
  std::shared_ptr<vsomeip::application> application;
  MessageHandler onMessage;
  std::atomic<bool> running{false};
  bool verboseLogging{false};
  std::jthread runtimeThread;

  auto onState(vsomeip::state_type_e state) -> void {
    if (state == vsomeip::state_type_e::ST_REGISTERED) {
      application->request_service(config.serviceId, config.instanceId);
      if (verboseLogging) {
        sdv::logLine("VecuSubscriber requested configured service");
      }
    }
  }

  auto onAvailability(vsomeip::service_t, vsomeip::instance_t, bool isAvailable) -> void {
    if (verboseLogging) {
      sdv::logLine(isAvailable ? "VecuSubscriber service AVAILABLE"
                               : "VecuSubscriber service UNAVAILABLE");
    }
  }

  auto onMessageInternal(const std::shared_ptr<vsomeip::message>& message) -> void {
    if (message->get_method() != config.eventId) {
      return;
    }

    const auto payload = message->get_payload();
    const auto* data = payload->get_data();
    const auto length = payload->get_length();
    std::vector<std::uint8_t> bytes{data, data + length};
    if (onMessage) {
      onMessage(bytes);
    }
  }

  auto sendMethodRequest(vsomeip::method_t method, const std::vector<vsomeip::byte_t>& bytes)
      -> void {
    if (!application) {
      return;
    }
    auto request = vsomeip::runtime::get()->create_request(false);
    request->set_service(config.serviceId);
    request->set_instance(config.instanceId);
    request->set_method(method);
    auto requestPayload = vsomeip::runtime::get()->create_payload();
    if (!bytes.empty()) {
      requestPayload->set_data(bytes);
    }
    request->set_payload(requestPayload);
    application->send(request);
  }
};

VecuSubscriber::VecuSubscriber() : impl_{std::make_unique<Impl>()} {}

VecuSubscriber::~VecuSubscriber() {
  stop();
}

VecuSubscriber::VecuSubscriber(VecuSubscriber&&) noexcept = default;

auto VecuSubscriber::operator=(VecuSubscriber&&) noexcept -> VecuSubscriber& = default;

auto VecuSubscriber::start(
    const VecuServiceConfig& config,
    MessageHandler onMessage,
    bool verboseLogging) -> bool {
  if (impl_->running.load()) {
    return true;
  }

  impl_->config = config;
  impl_->verboseLogging = resolveVerbose(verboseLogging);
  applyVsomeipConfiguration(impl_->config, impl_->verboseLogging);
  impl_->onMessage = std::move(onMessage);

  impl_->application =
      vsomeip::runtime::get()->create_application(impl_->config.applicationName);

  if (!impl_->application->init()) {
    sdv::logLine("VecuSubscriber failed to initialise vsomeip application");
    return false;
  }

  impl_->application->register_state_handler(
      [this](vsomeip::state_type_e state) { impl_->onState(state); });
  impl_->application->register_availability_handler(
      impl_->config.serviceId,
      impl_->config.instanceId,
      [this](vsomeip::service_t service, vsomeip::instance_t instance, bool isAvailable) {
        impl_->onAvailability(service, instance, isAvailable);
      });
  impl_->application->register_message_handler(
      impl_->config.serviceId,
      impl_->config.instanceId,
      vsomeip::ANY_METHOD,
      [this](const std::shared_ptr<vsomeip::message>& message) {
        impl_->onMessageInternal(message);
      });

  std::set<vsomeip::eventgroup_t> eventGroups{impl_->config.eventGroupId};
  impl_->application->request_event(
      impl_->config.serviceId,
      impl_->config.instanceId,
      impl_->config.eventId,
      eventGroups,
      vsomeip::event_type_e::ET_FIELD,
      vsomeip::reliability_type_e::RT_UNRELIABLE);
  impl_->application->subscribe(
      impl_->config.serviceId,
      impl_->config.instanceId,
      impl_->config.eventGroupId);

  impl_->running.store(true);
  impl_->runtimeThread = std::jthread([this](std::stop_token /*stopToken*/) {
    impl_->application->start();
  });

  if (impl_->verboseLogging) {
    sdv::logLine("VecuSubscriber runtime started");
  }
  return true;
}

auto VecuSubscriber::start(MessageHandler onMessage, bool verboseLogging) -> bool {
  return start(defaultSensorSubscriberConfig(), std::move(onMessage), verboseLogging);
}

auto VecuSubscriber::stop() -> void {
  if (!impl_ || !impl_->running.exchange(false)) {
    return;
  }

  if (impl_->application) {
    impl_->application->unsubscribe(
        impl_->config.serviceId,
        impl_->config.instanceId,
        impl_->config.eventGroupId);
    impl_->application->release_event(
        impl_->config.serviceId,
        impl_->config.instanceId,
        impl_->config.eventId);
    impl_->application->release_service(impl_->config.serviceId, impl_->config.instanceId);
    impl_->application->clear_all_handler();
    impl_->application->stop();
  }

  if (impl_->runtimeThread.joinable()) {
    impl_->runtimeThread.request_stop();
  }
}

auto VecuSubscriber::requestSetIntervalMs(std::uint32_t intervalMs) -> void {
  std::vector<vsomeip::byte_t> bytes(sizeof(intervalMs));
  std::memcpy(bytes.data(), &intervalMs, sizeof(intervalMs));
  impl_->sendMethodRequest(
      methodOrDefault(impl_->config, 0, sdv::ids::setIntervalMethodId),
      bytes);
  if (impl_->verboseLogging) {
    sdv::logLine("VecuSubscriber requested interval " + std::to_string(intervalMs) + " ms");
  }
}

auto VecuSubscriber::requestResetSequence() -> void {
  impl_->sendMethodRequest(
      methodOrDefault(impl_->config, 1, sdv::ids::resetSequenceMethodId),
      {});
  if (impl_->verboseLogging) {
    sdv::logLine("VecuSubscriber requested sequence reset");
  }
}

auto VecuSubscriber::requestSetStreaming(bool enabled) -> void {
  impl_->sendMethodRequest(
      methodOrDefault(impl_->config, 2, sdv::ids::setStreamingMethodId),
      {enabled ? vsomeip::byte_t{1} : vsomeip::byte_t{0}});
  if (impl_->verboseLogging) {
    sdv::logLine(enabled ? "VecuSubscriber requested streaming ON"
                         : "VecuSubscriber requested streaming OFF");
  }
}

auto wantsVerboseLoggingFromArgs(int argc, char** argv) -> bool {
  if (envFlagEnabled("VECU_VERBOSE")) {
    return true;
  }

  for (int index = 1; index < argc; ++index) {
    const std::string argument{argv[index]};
    if (argument == "--verbose" || argument == "-v") {
      return true;
    }
  }
  return false;
}

}  // namespace vecu
