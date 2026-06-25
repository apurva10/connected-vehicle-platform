#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace cvp {

// ── ConnectivityRuntime compile-time constants ───────────────────────────────
inline constexpr std::string_view kDefaultBrokerHost{"localhost"};
inline constexpr std::uint16_t    kDefaultBrokerPort{1883};
inline constexpr std::string_view kDefaultClientId{"cvp-client"};
// ─────────────────────────────────────────────────────────────────────────────

struct ConnectivityConfig {
  std::string   brokerHost{kDefaultBrokerHost};
  std::uint16_t brokerPort{kDefaultBrokerPort};
  std::string   clientId{kDefaultClientId};
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
