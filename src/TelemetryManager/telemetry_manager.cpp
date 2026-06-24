#include "connected_vehicle_platform/telemetry_manager.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>

namespace cvp {
namespace {

std::string currentTimestamp() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&time, &tm);
  std::ostringstream ss;
  ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
  return ss.str();
}

}  // namespace

std::string TelemetrySample::serialize() const {
  std::ostringstream out;
  out << "{\"vehicle_id\":\"" << vehicle_id << "\",\"timestamp\":\"" << timestamp
      << "\",\"speed\":" << speed << ",\"rpm\":" << rpm << ",\"battery\":" << battery
      << ",\"fuel\":" << fuel << "}";
  return out.str();
}

TelemetryManager::TelemetryManager(std::string vehicle_id, std::shared_ptr<IMqttClient> mqtt_client)
    : vehicle_id_(std::move(vehicle_id)), mqtt_client_(std::move(mqtt_client)) {}

TelemetrySample TelemetryManager::generateSample() const {
  return TelemetrySample{vehicle_id_, currentTimestamp(), 80.0, 2500.0, 12.6, 70.0};
}

std::string TelemetryManager::serialize(const TelemetrySample& sample) const {
  return sample.serialize();
}

bool TelemetryManager::publishSample() {
  TelemetrySample sample = generateSample();
  const std::string payload = serialize(sample);
  if (mqtt_client_ && mqtt_client_->isConnected()) {
    const bool ok = mqtt_client_->publish(mqtt_client_->config().telemetry_topic, payload, 1);
    if (!ok) {
      return queueMessage(payload);
    }
    return true;
  }
  return queueMessage(payload);
}

bool TelemetryManager::queueMessage(std::string message) {
  std::scoped_lock lock(mutex_);
  pending_messages_.push_back(std::move(message));
  return true;
}

bool TelemetryManager::flushQueue() {
  std::scoped_lock lock(mutex_);
  if (!mqtt_client_ || !mqtt_client_->isConnected()) {
    return false;
  }

  while (!pending_messages_.empty()) {
    const std::string payload = pending_messages_.front();
    pending_messages_.pop_front();
    const bool ok = mqtt_client_->publish(mqtt_client_->config().telemetry_topic, payload, 1);
    if (!ok) {
      return false;
    }
  }
  return true;
}

std::size_t TelemetryManager::pendingCount() const {
  std::scoped_lock lock(mutex_);
  return pending_messages_.size();
}

}  // namespace cvp
