#include <gtest/gtest.h>

#include "connected_vehicle_platform/connectivity_service.hpp"

TEST(ConnectivityServiceTests, InitializesAndPublishesTelemetry) {
  cvp::ConnectivityService service{"tcp://vehicle-broker.local:1883", "token-123", "vehicle-01"};

  EXPECT_TRUE(service.initialize());
  EXPECT_TRUE(service.publishTelemetry("engine_rpm", 3200.0));
}
