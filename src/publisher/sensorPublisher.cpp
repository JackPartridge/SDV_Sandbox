#include "sensorPublisher.hpp"

#include "../common/consoleLog.hpp"
#include "../common/serviceIds.hpp"
#include "../common/telemetryFrame.hpp"

#include <cstring>
#include <iomanip>
#include <set>
#include <sstream>
#include <vector>

namespace sdv {

SensorPublisher::SensorPublisher(std::chrono::milliseconds publishInterval)
    : application_{vsomeip::runtime::get()->create_application(ids::publisherApplicationName)},
      payload_{vsomeip::runtime::get()->create_payload()},
      publishIntervalMs_{publishInterval.count()} {}

auto SensorPublisher::init() -> bool {
  if (!application_->init()) {
    logLine("Publisher failed to initialise vsomeip application");
    return false;
  }

  application_->register_state_handler([this](vsomeip::state_type_e state) { onState(state); });
  application_->register_message_handler(
      ids::sensorServiceId,
      ids::sensorInstanceId,
      vsomeip::ANY_METHOD,
      [this](const std::shared_ptr<vsomeip::message>& message) { onMessage(message); });

  std::set<vsomeip::eventgroup_t> eventGroups{ids::sensorEventGroupId};
  application_->offer_event(
      ids::sensorServiceId,
      ids::sensorInstanceId,
      ids::sensorEventId,
      eventGroups,
      vsomeip::event_type_e::ET_FIELD,
      std::chrono::milliseconds::zero(),
      false,
      true,
      nullptr,
      vsomeip::reliability_type_e::RT_UNRELIABLE);

  return true;
}

auto SensorPublisher::start() -> void {
  publishThread_ = std::jthread([this](std::stop_token stopToken) { publishLoop(stopToken); });
  application_->start();
}

auto SensorPublisher::stop() -> void {
  if (publishThread_.joinable()) {
    publishThread_.request_stop();
  }
  if (offered_) {
    application_->stop_offer_service(ids::sensorServiceId, ids::sensorInstanceId);
    offered_ = false;
  }
  application_->clear_all_handler();
  application_->stop();
}

auto SensorPublisher::onState(vsomeip::state_type_e state) -> void {
  if (state == vsomeip::state_type_e::ST_REGISTERED) {
    logLine("Publisher REGISTERED with routing manager as '" + application_->get_name() + "'");
    application_->offer_service(ids::sensorServiceId, ids::sensorInstanceId);
    offered_ = true;

    std::ostringstream line;
    line << std::hex << std::setfill('0')
         << "Publisher OFFER service=0x" << std::setw(4) << ids::sensorServiceId
         << " instance=0x" << std::setw(4) << ids::sensorInstanceId
         << " event=0x" << std::setw(4) << ids::sensorEventId
         << " methods=setInterval/reset/setStreaming";
    logLine(line.str());
  }
}

auto SensorPublisher::sendMethodResponse(
    const std::shared_ptr<vsomeip::message>& request,
    const std::vector<vsomeip::byte_t>& payloadBytes) -> void {
  auto response = vsomeip::runtime::get()->create_response(request);
  auto responsePayload = vsomeip::runtime::get()->create_payload();
  responsePayload->set_data(payloadBytes);
  response->set_payload(responsePayload);
  application_->send(response);
}

auto SensorPublisher::onMessage(const std::shared_ptr<vsomeip::message>& message) -> void {
  if (message->get_message_type() != vsomeip::message_type_e::MT_REQUEST) {
    return;
  }

  const auto method = message->get_method();
  if (method == ids::setIntervalMethodId) {
    const auto payload = message->get_payload();
    if (payload->get_length() >= 4) {
      std::uint32_t intervalMs{0};
      std::memcpy(&intervalMs, payload->get_data(), sizeof(intervalMs));
      if (intervalMs < 50) {
        intervalMs = 50;
      }
      if (intervalMs > 10'000) {
        intervalMs = 10'000;
      }
      publishIntervalMs_.store(static_cast<std::int64_t>(intervalMs));
      logLine("Publisher interval set to " + std::to_string(intervalMs) + " ms");
    }
    sendMethodResponse(message, {0x00});
    return;
  }

  if (method == ids::resetSequenceMethodId) {
    sequence_.store(0);
    logLine("Publisher sequence counter reset");
    sendMethodResponse(message, {0x00});
    return;
  }

  if (method == ids::setStreamingMethodId) {
    const auto payload = message->get_payload();
    const bool enabled = payload->get_length() > 0 && payload->get_data()[0] != 0;
    streamingEnabled_.store(enabled);
    logLine(enabled ? "Publisher streaming ENABLED" : "Publisher streaming PAUSED (fault simulation)");
    sendMethodResponse(message, {enabled ? vsomeip::byte_t{0x01} : vsomeip::byte_t{0x00}});
  }
}

auto SensorPublisher::publishLoop(std::stop_token stopToken) -> void {
  while (!offered_ && !stopToken.stop_requested()) {
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
  }

  while (!stopToken.stop_requested()) {
    if (!offered_ || !streamingEnabled_.load()) {
      std::this_thread::sleep_for(std::chrono::milliseconds{50});
      continue;
    }

    const auto sequence = sequence_.fetch_add(1);
    const auto frame = collector_.sample(sequence);
    const auto bytes = serialiseTelemetryFrame(frame);
    payload_->set_data(bytes.data(), static_cast<vsomeip::length_t>(bytes.size()));
    application_->notify(ids::sensorServiceId, ids::sensorInstanceId, ids::sensorEventId, payload_);

    const auto interval = std::chrono::milliseconds{publishIntervalMs_.load()};
    std::this_thread::sleep_for(interval);
  }
}

}  // namespace sdv
