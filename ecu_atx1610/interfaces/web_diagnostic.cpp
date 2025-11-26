#include "web_diagnostic.hpp"

namespace interfaces {

void WebDiagnostic::log_snapshot(const common::SystemSnapshot &snapshot)
{
    LOG_DEBUG("WebDiagnostic", "Publicando snapshot con SOC=" + std::to_string(snapshot.battery.state_of_charge));
}

} // namespace interfaces
