#pragma once

#include <cstdint>
#include <string>

namespace cvp {

struct ConnectivityConfig {
  std::string brokerHost{"localhost"};
  std::uint16_t brokerPort{1883};
  std::string clientId{"cvp-client"};
};

class ConnectivityRuntime {
 public:
  explicit ConnectivityRuntime(ConnectivityConfig config);

  bool connect();
  bool disconnect();
  bool isConnected() const;
  const ConnectivityConfig& config() const;

 private:
  ConnectivityConfig config_;
  bool connected_{false};
};

}  // namespace cvp
