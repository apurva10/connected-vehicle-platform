#include "connected_vehicle_platform/mqtt_client.hpp"

#include <chrono>
#include <mutex>
#include <utility>

#include <mqtt/async_client.h>

namespace cvp {
namespace {

constexpr int kMqttKeepAliveSeconds{20};
constexpr int kMqttOperationTimeoutSec{5};

class PahoMqttBackend : public IMqttBackend {
 public:
  bool connect(std::string_view broker, std::string_view client_id) override {
    std::scoped_lock lock(mutex_);
    if (connected_) {
      return true;
    }

    try {
      client_ = std::make_unique<mqtt::async_client>(std::string{broker}, std::string{client_id});
      mqtt::connect_options options;
      options.set_keep_alive_interval(kMqttKeepAliveSeconds);
      options.set_clean_session(true);
      auto tok = client_->connect(options);
      tok->wait_for(std::chrono::seconds{kMqttOperationTimeoutSec});
      connected_ = client_->is_connected();
      return connected_;
    } catch (const std::exception&) {
      connected_ = false;
      return false;
    }
  }

  bool disconnect() override {
    std::scoped_lock lock(mutex_);
    try {
      if (client_) {
        auto tok = client_->disconnect();
        tok->wait_for(std::chrono::seconds{kMqttOperationTimeoutSec});
      }
    } catch (const std::exception&) {
      connected_ = false;
      return false;
    }

    connected_ = false;
    return true;
  }

  bool publish(std::string_view topic, std::string_view payload, int qos) override {
    std::scoped_lock lock(mutex_);
    if (!client_ || !connected_) {
      return false;
    }

    try {
      auto tok = client_->publish(std::string{topic}, std::string{payload}, static_cast<int>(qos), false);
      tok->wait_for(std::chrono::seconds{kMqttOperationTimeoutSec});
      return tok->get_return_code() == mqtt::SUCCESS;
    } catch (const std::exception&) {
      return false;
    }
  }

  bool subscribe(std::string_view topic, int qos) override {
    std::scoped_lock lock(mutex_);
    if (!client_ || !connected_) {
      return false;
    }

    try {
      auto tok = client_->subscribe(std::string{topic}, static_cast<int>(qos));
      tok->wait_for(std::chrono::seconds{kMqttOperationTimeoutSec});
      return tok->get_return_code() == mqtt::SUCCESS;
    } catch (const std::exception&) {
      return false;
    }
  }

  bool isConnected() const override {
    std::scoped_lock lock(mutex_);
    return connected_ && client_ && client_->is_connected();
  }

 private:
  mutable std::mutex mutex_;
  std::unique_ptr<mqtt::async_client> client_;
  bool connected_{false};
};

}  // namespace

MqttClient::MqttClient(MqttConfig config, std::shared_ptr<IMqttBackend> backend)
    : config_(std::move(config)), backend_(std::move(backend)) {
  if (!backend_) {
    backend_ = std::make_shared<PahoMqttBackend>();
  }
}

bool MqttClient::connect() {
  std::scoped_lock lock(mutex_);
  if (connected_) {
    return true;
  }

  connected_ = backend_->connect(config_.broker, config_.client_id);
  return connected_;
}

bool MqttClient::disconnect() {
  std::scoped_lock lock(mutex_);
  connected_ = false;
  return backend_->disconnect();
}

bool MqttClient::publish(std::string_view topic, std::string_view payload, int qos) {
  std::scoped_lock lock(mutex_);
  if (!connected_) {
    return false;
  }
  return backend_->publish(topic, payload, qos < 0 ? config_.qos : qos);
}

bool MqttClient::subscribe(std::string_view topic, int qos) {
  std::scoped_lock lock(mutex_);
  if (!connected_) {
    return false;
  }
  return backend_->subscribe(topic, qos < 0 ? config_.qos : qos);
}

bool MqttClient::isConnected() const {
  std::scoped_lock lock(mutex_);
  return connected_;
}

const MqttConfig& MqttClient::config() const {
  return config_;
}

bool MqttClient::reconnect() {
  return disconnect() && connect();
}

}  // namespace cvp
