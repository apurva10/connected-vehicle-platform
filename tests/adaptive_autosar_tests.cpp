#include <gtest/gtest.h>

#include "connected_vehicle_platform/vehicle_signal_service.hpp"

namespace cvp::tests {

TEST(AdaptiveAutosarTests, VehicleSignalServiceProvidesVehicleSnapshot) {
  VehicleSignalService service;

  const auto snapshot = service.readSignalSnapshot();

  EXPECT_EQ(snapshot.speed, 0U);
  EXPECT_DOUBLE_EQ(snapshot.batteryVoltage, 12.4);
}

}  // namespace cvp::tests
