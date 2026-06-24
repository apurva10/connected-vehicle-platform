#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>

#include "connected_vehicle_platform/mqtt_client.hpp"

namespace cvp {

struct TelemetrySample {
  std::string vehicle_id;
  std::string timestamp;
  double speed{0.0};
  double rpm{0.0};
  double battery{0.0};
  double fuel{0.0};

  std::string serialize() const;
};

class TelemetryManager {
 public:
  TelemetryManager(std::string vehicle_id, std::shared_ptr<IMqttClient> mqtt_client);

  TelemetrySample generateSample() const;
  std::string serialize(const TelemetrySample& sample) const;
  bool publishSample();
  bool queueMessage(std::string message);
  bool flushQueue();
  [[nodiscard]] std::size_t pendingCount() const;

 private:
  std::string vehicle_id_;
  std::shared_ptr<IMqttClient> mqtt_client_;
  mutable std::mutex mutex_;
  std::deque<std::string> pending_messages_;
};

}  // namespace cvp
