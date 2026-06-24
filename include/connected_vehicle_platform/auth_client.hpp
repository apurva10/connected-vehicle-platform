#pragma once

#include <chrono>
#include <memory>
#include <string>

namespace cvp {

struct AuthRequest {
  std::string vehicle_id;
  std::string vehicle_secret;
};

struct AuthResponse {
  std::string access_token;
  std::chrono::seconds expires_in{0};
};

class IAuthBackend {
 public:
  virtual ~IAuthBackend() = default;
  virtual AuthResponse authenticate(const AuthRequest& request) = 0;
};

class AuthClient {
 public:
  AuthClient(std::shared_ptr<IAuthBackend> backend, std::string vehicle_id,
             std::string vehicle_secret);

  bool authenticate();
  bool refreshToken();
  bool isTokenValid() const;
  void clearToken() noexcept;

  [[nodiscard]] const std::string& token() const noexcept;
  [[nodiscard]] std::chrono::steady_clock::time_point expiresAt() const noexcept;

 private:
  std::shared_ptr<IAuthBackend> backend_;
  std::string vehicle_id_;
  std::string vehicle_secret_;
  std::string access_token_;
  std::chrono::steady_clock::time_point expires_at_;
};

}  // namespace cvp
