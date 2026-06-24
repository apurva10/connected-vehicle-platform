#include "connected_vehicle_platform/connectivity_runtime.hpp"

#include <utility>

namespace cvp {

ConnectivityRuntime::ConnectivityRuntime(ConnectivityConfig config) : config_(std::move(config)) {}

bool ConnectivityRuntime::connect() {
  connected_ = !config_.brokerHost.empty();
  return connected_;
}

bool ConnectivityRuntime::disconnect() {
  connected_ = false;
  return true;
}

bool ConnectivityRuntime::isConnected() const {
  return connected_;
}

const ConnectivityConfig& ConnectivityRuntime::config() const {
  return config_;
}

}  // namespace cvp
