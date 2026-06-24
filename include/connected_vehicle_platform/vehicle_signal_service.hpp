#pragma once

#include <cstdint>

namespace cvp {

struct VehicleSignalSnapshot {
  std::uint16_t speed{0};
  double batteryVoltage{12.4};
};

class VehicleSignalService {
 public:
  VehicleSignalSnapshot readSignalSnapshot() const;
};

}  // namespace cvp
