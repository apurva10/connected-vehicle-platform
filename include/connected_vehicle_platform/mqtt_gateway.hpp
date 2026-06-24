#pragma once

#include <string>

namespace cvp {

class MqttGateway {
 public:
  explicit MqttGateway(std::string topic = "vehicle/signals");

  bool publish(const std::string& payload) const;
  bool subscribe(const std::string& topic) const;
  const std::string& topic() const;

 private:
  std::string topic_;
};

}  // namespace cvp
