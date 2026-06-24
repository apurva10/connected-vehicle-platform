#pragma once

#include <string>

namespace cvp {

class AuthSession {
 public:
  explicit AuthSession(std::string tenant = "vehicle");

  bool authenticate(const std::string& token);
  bool isAuthenticated() const;
  const std::string& tenant() const;

 private:
  std::string tenant_;
  bool authenticated_{false};
};

}  // namespace cvp
