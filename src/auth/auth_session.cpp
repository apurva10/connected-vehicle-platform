#include "connected_vehicle_platform/auth_session.hpp"
#include <thread>
#include <chrono>
#include <utility>

namespace cvp {

AuthSession::AuthSession(std::string tenant) : tenant_(std::move(tenant)) {}

bool AuthSession::authenticate(const std::string& token,
                               std::function<std::string()> token_provider,
                               int max_retries) {
  token_ = token;
  int attempts = 0;

  while (token_.empty() && token_provider && attempts < max_retries) {
    attempts++;
    token_ = token_provider();
    if (token_.empty() && attempts < max_retries) {
      std::this_thread::sleep_for(std::chrono::milliseconds(kRetryDelayMs));
    }
  }

  authenticated_ = !token_.empty();
  return authenticated_;
}

bool AuthSession::isAuthenticated() const {
  return authenticated_;
}

const std::string& AuthSession::tenant() const {
  return tenant_;
}

const std::string& AuthSession::token() const {
  return token_;
}

}  // namespace cvp
