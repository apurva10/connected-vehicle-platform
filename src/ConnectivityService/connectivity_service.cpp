#include "connected_vehicle_platform/connectivity_service.hpp"

#include <chrono>
#include <utility>

namespace cvp {

namespace {
constexpr int kReconnectDelaySec{2};
constexpr int kPublishIntervalSec{5};
}  // namespace


ConnectivityService::ConnectivityService(std::shared_ptr<AuthClient> auth_client,
                                         std::shared_ptr<IMqttClient> mqtt_client,
                                         std::shared_ptr<TelemetryManager> telemetry_manager,
                                         std::shared_ptr<Logger> logger)
    : auth_client_(std::move(auth_client)),
      mqtt_client_(std::move(mqtt_client)),
      telemetry_manager_(std::move(telemetry_manager)),
      logger_(std::move(logger)) {}

bool ConnectivityService::start() {
  if (running_.exchange(true)) {
    return true;
  }

  state_.store(ServiceState::Starting);
  logger_->info("Starting connectivity service");

  if (!auth_client_ || !auth_client_->authenticate()) {
    logger_->error("Authentication failed");
    state_.store(ServiceState::Disconnected);
    running_.store(false);
    return false;
  }

  state_.store(ServiceState::Authenticating);
  logger_->info("Authentication succeeded");

  if (!mqtt_client_ || !mqtt_client_->connect()) {
    logger_->error("MQTT connection failed");
    state_.store(ServiceState::Disconnected);
    running_.store(false);
    return false;
  }

  state_.store(ServiceState::Connected);
  logger_->info("Cloud transport established");

  worker_ = std::jthread([this](std::stop_token stop_token) { runLoop(stop_token); });
  state_.store(ServiceState::Publishing);
  return true;
}

bool ConnectivityService::stop() {
  if (!running_.exchange(false)) {
    return true;
  }

  logger_->info("Stopping connectivity service");
  worker_.request_stop();
  worker_.join();
  state_.store(ServiceState::Stopped);
  return true;
}

bool ConnectivityService::publishTelemetry() {
  if (!telemetry_manager_) {
    return false;
  }

  return telemetry_manager_->publishSample();
}

ServiceState ConnectivityService::state() const noexcept {
  return state_.load();
}

bool ConnectivityService::running() const noexcept {
  return running_.load();
}

void ConnectivityService::runLoop(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    if (auth_client_ && (!auth_client_->isTokenValid() || auth_client_->token().empty())) {
      state_.store(ServiceState::Authenticating);
      logger_->warning("Token is empty or invalid; attempting to refresh token");
      if (auth_client_->refreshToken() && !auth_client_->token().empty()) {
        logger_->info("Token refreshed successfully");
        state_.store(ServiceState::Publishing);
      } else {
        logger_->error("Token refresh failed; retrying");
        state_.store(ServiceState::Disconnected);
        std::this_thread::sleep_for(std::chrono::seconds(kReconnectDelaySec));
        continue;
      }
    }

    if (!mqtt_client_ || !mqtt_client_->isConnected()) {
      state_.store(ServiceState::Reconnecting);
      logger_->warning("Connection lost; attempting reconnect");
      if (mqtt_client_ && mqtt_client_->reconnect()) {
        state_.store(ServiceState::Connected);
        logger_->info("Reconnected successfully");
      } else {
        state_.store(ServiceState::Disconnected);
        std::this_thread::sleep_for(std::chrono::seconds(kReconnectDelaySec));
        continue;
      }
    }

    if (!publishTelemetry()) {
      logger_->warning("Telemetry publish queued for later");
    }

    std::this_thread::sleep_for(std::chrono::seconds(kPublishIntervalSec));
  }

  if (mqtt_client_) {
    mqtt_client_->disconnect();
  }
  logger_->info("Connectivity service loop stopped");
}

}  // namespace cvp
