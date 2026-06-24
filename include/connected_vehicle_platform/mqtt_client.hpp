#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <string_view>

namespace cvp {

struct MqttConfig {
  std::string broker;
  std::string client_id;
  std::string telemetry_topic;
  std::string command_topic;
  int qos{1};
};

class IMqttBackend {
 public:
  virtual ~IMqttBackend() = default;
  virtual bool connect(std::string_view broker, std::string_view client_id) = 0;
  virtual bool disconnect() = 0;
  virtual bool publish(std::string_view topic, std::string_view payload, int qos) = 0;
  virtual bool subscribe(std::string_view topic, int qos) = 0;
  virtual bool isConnected() const = 0;
};

class IMqttClient {
 public:
  virtual ~IMqttClient() = default;
  virtual bool connect() = 0;
  virtual bool disconnect() = 0;
  virtual bool reconnect() = 0;
  virtual bool publish(std::string_view topic, std::string_view payload, int qos = 1) = 0;
  virtual bool subscribe(std::string_view topic, int qos = 1) = 0;
  virtual bool isConnected() const = 0;
  virtual const MqttConfig& config() const = 0;
};

class MqttClient : public IMqttClient {
 public:
  explicit MqttClient(MqttConfig config,
                      std::shared_ptr<IMqttBackend> backend = nullptr);

  bool connect() override;
  bool disconnect() override;
  bool reconnect() override;
  bool publish(std::string_view topic, std::string_view payload, int qos = 1) override;
  bool subscribe(std::string_view topic, int qos = 1) override;
  bool isConnected() const override;
  const MqttConfig& config() const override;

 private:
  MqttConfig config_;
  std::shared_ptr<IMqttBackend> backend_;
  mutable std::mutex mutex_;
  bool connected_{false};
};

}  // namespace cvp

