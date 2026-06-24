#include "connected_vehicle_platform/auth_session.hpp"

#include <utility>

namespace cvp {

AuthSession::AuthSession(std::string tenant) : tenant_(std::move(tenant)) {}

bool AuthSession::authenticate(const std::string& token) {
  authenticated_ = !token.empty();
  return authenticated_;
}

bool AuthSession::isAuthenticated() const {
  return authenticated_;
}

const std::string& AuthSession::tenant() const {
  return tenant_;
}

}  // namespace cvp
