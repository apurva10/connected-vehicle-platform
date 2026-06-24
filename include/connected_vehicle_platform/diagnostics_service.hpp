#pragma once

#include <string>

namespace cvp {

struct DiagnosticReport {
  std::string code;
  std::string severity;
  std::string detail;
};

class DiagnosticsService {
 public:
  bool publishDiagnostic(const DiagnosticReport& report) const;
};

}  // namespace cvp
