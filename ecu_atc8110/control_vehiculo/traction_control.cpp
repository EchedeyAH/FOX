#include "controllers.hpp"
#include "../common/logging.hpp"
#include "../common/types.hpp"

#include <algorithm>

namespace control_vehiculo {

class TractionControl : public common::IController {
public:
    bool start() override
    {
        LOG_INFO("TractionControl", "Control de tracción en modo seguro");
        return true;
    }

    void stop() override
    {
        LOG_INFO("TractionControl", "Control de tracción detenido");
    }

    void update(common::SystemSnapshot &snapshot) override
    {
        const double pedal = std::clamp(snapshot.vehicle.accelerator, 0.0, 1.0);
        const double demand = pedal * snapshot.power.available_w;
        snapshot.power.demanded_w = demand;
        for (auto &motor : snapshot.motors) {
            motor.enabled = pedal > 0.05;
            motor.torque_nm = demand / snapshot.motors.size();
            motor.rpm = 1000.0 * pedal;
        }
    }
};

std::unique_ptr<common::IController> CreateTractionControl()
{
    return std::make_unique<TractionControl>();
}

} // namespace control_vehiculo
