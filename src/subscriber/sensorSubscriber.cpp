#include "sensorSubscriber.hpp"

#include "../common/consoleLog.hpp"
#include "../common/serviceIds.hpp"

#include <iomanip>
#include <set>
#include <sstream>

namespace sdv {

SensorSubscriber::SensorSubscriber(FrameHandler onFrame)
    : application_{vsomeip::runtime::get()->create_application(ids::subscriberApplicationName)},
      onFrame_{std::move(onFrame)} {}

auto SensorSubscriber::init() -> bool {
  if (!application_->init()) {
    logLine("Subscriber failed to initialise vsomeip application");
    return false;
  }

  application_->register_state_handler([this](vsomeip::state_type_e state) { onState(state); });
  application_->register_availability_handler(
      ids::sensorServiceId,
      ids::sensorInstanceId,
      [this](vsomeip::service_t service, vsomeip::instance_t instance, bool isAvailable) {
        onAvailability(service, instance, isAvailable);
      });
  application_->register_message_handler(
      ids::sensorServiceId,
      ids::sensorInstanceId,
      ids::sensorEventId,
      [this](const std::shared_ptr<vsomeip::message>& message) { onMessage(message); });

  std::set<vsomeip::eventgroup_t> eventGroups{ids::sensorEventGroupId};
  application_->request_event(
      ids::sensorServiceId,
      ids::sensorInstanceId,
      ids::sensorEventId,
      eventGroups,
      vsomeip::event_type_e::ET_FIELD,
      vsomeip::reliability_type_e::RT_UNRELIABLE);
  application_->subscribe(ids::sensorServiceId, ids::sensorInstanceId, ids::sensorEventGroupId);

  return true;
}

auto SensorSubscriber::start() -> void {
  application_->start();
}

auto SensorSubscriber::stop() -> void {
  application_->unsubscribe(ids::sensorServiceId, ids::sensorInstanceId, ids::sensorEventGroupId);
  application_->release_event(ids::sensorServiceId, ids::sensorInstanceId, ids::sensorEventId);
  application_->release_service(ids::sensorServiceId, ids::sensorInstanceId);
  application_->clear_all_handler();
  application_->stop();
}

auto SensorSubscriber::receivedCount() const -> std::size_t {
  return receivedCount_.load();
}

auto SensorSubscriber::lastFrame() const -> std::optional<TelemetryFrame> {
  std::scoped_lock lock{framesMutex_};
  return lastFrame_;
}

auto SensorSubscriber::rejectedCount() const -> std::size_t {
  return rejectedCount_.load();
}

auto SensorSubscriber::onState(vsomeip::state_type_e state) -> void {
  if (state == vsomeip::state_type_e::ST_REGISTERED) {
    logLine("Subscriber REGISTERED with routing manager as '" + application_->get_name() + "'");
    application_->request_service(ids::sensorServiceId, ids::sensorInstanceId);

    std::ostringstream line;
    line << std::hex << std::setfill('0')
         << "Subscriber REQUEST/FIND service=0x" << std::setw(4) << ids::sensorServiceId
         << " instance=0x" << std::setw(4) << ids::sensorInstanceId
         << " eventgroup=0x" << std::setw(4) << ids::sensorEventGroupId;
    logLine(line.str());
  }
}

auto SensorSubscriber::onAvailability(
    vsomeip::service_t service,
    vsomeip::instance_t instance,
    bool isAvailable) -> void {
  std::ostringstream line;
  line << std::hex << std::setfill('0')
       << "Subscriber availability service=0x" << std::setw(4) << service
       << " instance=0x" << std::setw(4) << instance
       << std::dec << (isAvailable ? " AVAILABLE (subscription active)" : " UNAVAILABLE");
  logLine(line.str());
}

auto SensorSubscriber::onMessage(const std::shared_ptr<vsomeip::message>& message) -> void {
  const auto payload = message->get_payload();
  const auto* data = payload->get_data();
  const auto length = payload->get_length();
  const std::span<const std::uint8_t> bytes{data, data + length};

  const auto frame = processTelemetryPayload(bytes);
  if (!frame.has_value()) {
    rejectedCount_.fetch_add(1);
    logLine("Rejected invalid telemetry payload (" + std::to_string(length) + " bytes)");
    return;
  }

  {
    std::scoped_lock lock{framesMutex_};
    lastFrame_ = frame;
  }
  receivedCount_.fetch_add(1);

  {
    std::ostringstream line;
    line << "Decoded telemetry seq=" << frame->sequenceNumber
         << " cpuMilli%=" << frame->cpuUtilisationMilliPercent
         << " memMilli%=" << frame->memoryUsedMilliPercent
         << " tempMilliC=" << frame->temperatureMilliCelsius;
    logLine(line.str());
  }

  if (onFrame_) {
    onFrame_(*frame);
  }
}

}  // namespace sdv
