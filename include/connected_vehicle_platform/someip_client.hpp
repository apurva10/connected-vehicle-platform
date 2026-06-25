#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

#include "connected_vehicle_platform/mqtt_client.hpp"

namespace cvp {

// ── SomeipClient compile-time constants ──────────────────────────────────────
inline constexpr std::string_view kDefaultSomeipHost{"127.0.0.1"};
inline constexpr unsigned short   kDefaultSomeipPort{30509};
inline constexpr std::uint16_t    kDefaultServiceId{0x1234};
inline constexpr std::uint16_t    kDefaultMethodId{0x0001};
inline constexpr std::string_view kDefaultEcuClientId{"ECU_001"};
inline constexpr int              kInvalidSocketFd{-1};
// ─────────────────────────────────────────────────────────────────────────────

struct SomeipConfig {
  std::string    host{kDefaultSomeipHost};
  unsigned short port{kDefaultSomeipPort};
  std::uint16_t  service_id{kDefaultServiceId};
  std::uint16_t  method_id{kDefaultMethodId};
  std::string    client_id{kDefaultEcuClientId};
};

class SomeipClient : public IMqttClient {
 public:
  explicit SomeipClient(SomeipConfig config);

  bool connect() override;
  bool disconnect() override;
  bool reconnect() override;
  bool publish(std::string_view topic, std::string_view payload, int qos = kDefaultQos) override;
  bool publish(std::string_view payload);
  bool subscribe(std::string_view topic, int qos = kDefaultQos) override;
  bool isConnected() const override;
  const MqttConfig& config() const override;

  [[nodiscard]] const SomeipConfig& someipConfig() const noexcept;

 private:
  SomeipConfig someip_config_;
  MqttConfig mqtt_config_;
  mutable std::mutex mutex_;
  int socket_fd_{kInvalidSocketFd};
  bool connected_{false};
};

}  // namespace cvp
