#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

#include "connected_vehicle_platform/mqtt_client.hpp"

namespace cvp {

struct SomeipConfig {
  std::string host{"127.0.0.1"};
  unsigned short port{30509};
  std::uint16_t service_id{0x1234};
  std::uint16_t method_id{0x0001};
  std::string client_id{"ECU_001"};
};

class SomeipClient : public IMqttClient {
 public:
  explicit SomeipClient(SomeipConfig config);

  bool connect() override;
  bool disconnect() override;
  bool reconnect() override;
  bool publish(std::string_view topic, std::string_view payload, int qos = 1) override;
  bool publish(std::string_view payload);
  bool subscribe(std::string_view topic, int qos = 1) override;
  bool isConnected() const override;
  const MqttConfig& config() const override;

  [[nodiscard]] const SomeipConfig& someipConfig() const noexcept;

 private:
  SomeipConfig someip_config_;
  MqttConfig mqtt_config_;
  mutable std::mutex mutex_;
  int socket_fd_{-1};
  bool connected_{false};
};

}  // namespace cvp
