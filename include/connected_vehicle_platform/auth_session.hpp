#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace cvp {

// ── AuthSession compile-time constants ──────────────────────────────────────
inline constexpr std::string_view kDefaultTenant{"vehicle"};
inline constexpr int             kDefaultMaxRetries{3};
inline constexpr int             kRetryDelayMs{200};
// ────────────────────────────────────────────────────────────────────────────

class AuthSession {
 public:
  explicit AuthSession(std::string tenant = std::string{kDefaultTenant});

  bool authenticate(const std::string& token,
                    std::function<std::string()> token_provider = nullptr,
                    int max_retries = kDefaultMaxRetries);

  bool isAuthenticated() const;
  const std::string& tenant() const;
  const std::string& token() const;

 private:
  std::string tenant_;
  std::string token_;
  bool authenticated_{false};
};

}  // namespace cvp
