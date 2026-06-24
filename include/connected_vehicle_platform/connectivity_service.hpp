#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "connected_vehicle_platform/auth_client.hpp"
#include "connected_vehicle_platform/logger.hpp"
#include "connected_vehicle_platform/mqtt_client.hpp"
#include "connected_vehicle_platform/telemetry_manager.hpp"

namespace cvp {

enum class ServiceState {
  Starting,
  Authenticating,
  Connected,
  Publishing,
  Disconnected,
  Reconnecting,
  Stopped
};

class ConnectivityService {
 public:
  ConnectivityService(std::shared_ptr<AuthClient> auth_client,
                      std::shared_ptr<IMqttClient> mqtt_client,
                      std::shared_ptr<TelemetryManager> telemetry_manager,
                      std::shared_ptr<Logger> logger);

  bool start();
  bool stop();
  bool publishTelemetry();
  [[nodiscard]] ServiceState state() const noexcept;
  [[nodiscard]] bool running() const noexcept;

 private:
  void runLoop(std::stop_token stop_token);

  std::shared_ptr<AuthClient> auth_client_;
  std::shared_ptr<IMqttClient> mqtt_client_;
  std::shared_ptr<TelemetryManager> telemetry_manager_;
  std::shared_ptr<Logger> logger_;
  std::atomic<ServiceState> state_{ServiceState::Stopped};
  std::atomic<bool> running_{false};
  std::jthread worker_;
};

}  // namespace cvp
