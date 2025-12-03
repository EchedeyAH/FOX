#include "../common/interfaces.hpp"
#include "../common/logging.hpp"
#include "../common/types.hpp"

namespace interfaces {

class WebDiagnostic : public common::IDataLogger {
public:
    void log_snapshot(const common::SystemSnapshot &snapshot) override
    {
        LOG_DEBUG("WebDiagnostic", "Publicando snapshot con SOC=" + std::to_string(snapshot.battery.state_of_charge));
    }
};

} // namespace interfaces
