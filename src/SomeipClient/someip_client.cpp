#include "connected_vehicle_platform/someip_client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cvp {
namespace {

struct SomeipHeader {
  std::uint16_t service_id;
  std::uint16_t method_id;
  std::uint32_t length;
} __attribute__((packed));

}  // namespace

SomeipClient::SomeipClient(SomeipConfig config) : someip_config_(std::move(config)) {
  mqtt_config_ = {someip_config_.host, someip_config_.client_id, "vehicle/telemetry",
                  "vehicle/commands", 1};
}

bool SomeipClient::connect() {
  std::scoped_lock lock(mutex_);
  if (connected_) {
    return true;
  }

  socket_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    return false;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(someip_config_.port);
  if (inet_pton(AF_INET, someip_config_.host.c_str(), &address.sin_addr) != 1) {
    ::close(socket_fd_);
    socket_fd_ = -1;
    return false;
  }

  if (::connect(socket_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    ::close(socket_fd_);
    socket_fd_ = -1;
    return false;
  }

  connected_ = true;
  return true;
}

bool SomeipClient::disconnect() {
  std::scoped_lock lock(mutex_);
  if (socket_fd_ >= 0) {
    ::close(socket_fd_);
  }
  socket_fd_ = -1;
  connected_ = false;
  return true;
}

bool SomeipClient::reconnect() {
  disconnect();
  return connect();
}

bool SomeipClient::publish(std::string_view topic, std::string_view payload, int qos) {
  std::scoped_lock lock(mutex_);
  if (!connected_ || socket_fd_ < 0) {
    return false;
  }

  (void)topic;
  (void)qos;

  SomeipHeader header{someip_config_.service_id, someip_config_.method_id,
                      static_cast<std::uint32_t>(payload.size())};
  std::vector<char> frame(sizeof(header) + payload.size());
  std::memcpy(frame.data(), &header, sizeof(header));
  std::memcpy(frame.data() + sizeof(header), payload.data(), payload.size());

  const auto sent = ::send(socket_fd_, frame.data(), frame.size(), 0);
  return sent == static_cast<ssize_t>(frame.size());
}

bool SomeipClient::subscribe(std::string_view topic, int qos) {
  (void)topic;
  (void)qos;
  return isConnected();
}

bool SomeipClient::isConnected() const {
  std::scoped_lock lock(mutex_);
  return connected_ && socket_fd_ >= 0;
}

bool SomeipClient::publish(std::string_view payload) {
  return publish("", payload, 1);
}

const MqttConfig& SomeipClient::config() const {
  return mqtt_config_;
}

const SomeipConfig& SomeipClient::someipConfig() const noexcept {
  return someip_config_;
}

}  // namespace cvp
