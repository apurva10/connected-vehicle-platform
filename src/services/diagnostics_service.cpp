#include "connected_vehicle_platform/diagnostics_service.hpp"

namespace cvp {

bool DiagnosticsService::publishDiagnostic(const DiagnosticReport& report) const {
  return !report.code.empty() && !report.severity.empty();
}

}  // namespace cvp
