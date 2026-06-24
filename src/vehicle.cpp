#include "connected_vehicle_platform/vehicle.hpp"

#include <string>

namespace cvp {

std::string Vehicle::describe() const {
  return make + " " + model + " (" + std::to_string(year) + ")";
}

}  // namespace cvp
