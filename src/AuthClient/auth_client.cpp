#include "connected_vehicle_platform/auth_client.hpp"

#include <utility>

namespace cvp {

AuthClient::AuthClient(std::shared_ptr<IAuthBackend> backend, std::string vehicle_id,
                       std::string vehicle_secret)
    : backend_(std::move(backend)),
      vehicle_id_(std::move(vehicle_id)),
      vehicle_secret_(std::move(vehicle_secret)),
      expires_at_(std::chrono::steady_clock::now()) {}

bool AuthClient::authenticate() {
  if (!backend_) {
    return false;
  }

  const auto response = backend_->authenticate({vehicle_id_, vehicle_secret_});
  if (response.access_token.empty()) {
    access_token_.clear();
    expires_at_ = std::chrono::steady_clock::now();
    return false;
  }

  access_token_ = response.access_token;
  expires_at_ = std::chrono::steady_clock::now() + response.expires_in;
  return true;
}

bool AuthClient::refreshToken() {
  return authenticate();
}

bool AuthClient::isTokenValid() const {
  return !access_token_.empty() && std::chrono::steady_clock::now() < expires_at_;
}

void AuthClient::clearToken() noexcept {
  access_token_.clear();
  expires_at_ = std::chrono::steady_clock::now();
}

const std::string& AuthClient::token() const noexcept {
  return access_token_;
}

std::chrono::steady_clock::time_point AuthClient::expiresAt() const noexcept {
  return expires_at_;
}

}  // namespace cvp
