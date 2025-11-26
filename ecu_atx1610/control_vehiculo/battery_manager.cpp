#include "controllers.hpp"
#include "../common/logging.hpp"
#include "../common/types.hpp"

#include <algorithm>

namespace control_vehiculo {

class BatteryManager : public common::IController {
public:
    bool start() override
    {
        LOG_INFO("BatteryManager", "Monitorizando BMS");
        return true;
    }

    void stop() override
    {
        LOG_INFO("BatteryManager", "BMS detenido");
    }

    void update(common::SystemSnapshot &snapshot) override
    {
        snapshot.power.available_w = snapshot.battery.pack_voltage_mv * snapshot.battery.pack_current_ma / 1000.0;
        if (!snapshot.battery.communication_ok) {
            snapshot.faults.error = true;
            snapshot.faults.description = "BMS sin comunicación";
        }
        snapshot.battery.state_of_charge = std::clamp(snapshot.battery.state_of_charge, 0.0, 100.0);
    }
};

std::unique_ptr<common::IController> CreateBatteryManager()
{
    return std::make_unique<BatteryManager>();
}

} // namespace control_vehiculo
