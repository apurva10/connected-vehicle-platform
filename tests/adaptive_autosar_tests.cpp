#include <gtest/gtest.h>

#include "connected_vehicle_platform/auth_session.hpp"
#include "connected_vehicle_platform/vehicle_signal_service.hpp"

namespace cvp::tests {

TEST(AdaptiveAutosarTests, VehicleSignalServiceProvidesVehicleSnapshot) {
  VehicleSignalService service;

  const auto snapshot = service.readSignalSnapshot();

  EXPECT_EQ(snapshot.speed, 0U);
  EXPECT_DOUBLE_EQ(snapshot.batteryVoltage, 12.4);
}

TEST(AdaptiveAutosarTests, AuthSessionTokenRetryLogic) {
  AuthSession session("test-tenant");

  // 1. Success on initial non-empty token
  EXPECT_TRUE(session.authenticate("initial-token"));
  EXPECT_TRUE(session.isAuthenticated());
  EXPECT_EQ(session.token(), "initial-token");

  // 2. Success with empty token but provider returns token immediately
  int provider_calls = 0;
  auto provider_immediate = [&]() -> std::string {
    provider_calls++;
    return "provided-token-1";
  };
  EXPECT_TRUE(session.authenticate("", provider_immediate, 3));
  EXPECT_TRUE(session.isAuthenticated());
  EXPECT_EQ(session.token(), "provided-token-1");
  EXPECT_EQ(provider_calls, 1);

  // 3. Success after retries
  int retry_calls = 0;
  auto provider_retry = [&]() -> std::string {
    retry_calls++;
    if (retry_calls < 3) {
      return "";
    }
    return "provided-token-2";
  };
  EXPECT_TRUE(session.authenticate("", provider_retry, 3));
  EXPECT_TRUE(session.isAuthenticated());
  EXPECT_EQ(session.token(), "provided-token-2");
  EXPECT_EQ(retry_calls, 3);

  // 4. Failure after max retries exceeded
  int fail_calls = 0;
  auto provider_fail = [&]() -> std::string {
    fail_calls++;
    return "";
  };
  EXPECT_FALSE(session.authenticate("", provider_fail, 3));
  EXPECT_FALSE(session.isAuthenticated());
  EXPECT_EQ(session.token(), "");
  EXPECT_EQ(fail_calls, 3);
}

}  // namespace cvp::tests
