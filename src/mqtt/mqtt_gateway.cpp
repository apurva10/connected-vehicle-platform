#include "connected_vehicle_platform/mqtt_gateway.hpp"

#include <utility>

namespace cvp {

MqttGateway::MqttGateway(std::string topic) : topic_(std::move(topic)) {}

bool MqttGateway::publish(const std::string& payload) const {
  return !payload.empty();
}

bool MqttGateway::subscribe(const std::string& topic) const {
  return !topic.empty();
}

const std::string& MqttGateway::topic() const {
  return topic_;
}

}  // namespace cvp
