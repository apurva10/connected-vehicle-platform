#pragma once

#include <string>

namespace cvp {

struct Vehicle {
  std::string make;
  std::string model;
  int year{0};

  std::string describe() const;
};

}  // namespace cvp
