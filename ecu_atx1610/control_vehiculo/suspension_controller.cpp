#include "controllers.hpp"
#include "../common/logging.hpp"
#include "../common/types.hpp"

#include <algorithm>

namespace control_vehiculo {

class SuspensionController : public common::IController {
public:
    bool start() override
    {
        LOG_INFO("Suspension", "Inicializando controlador de suspensión");
        return true;
    }

    void stop() override
    {
        LOG_INFO("Suspension", "Controlador de suspensión detenido");
    }

    void update(common::SystemSnapshot &snapshot) override
    {
        const double target = 50.0; // mm
        for (auto &height : snapshot.vehicle.suspension_mm) {
            height = std::clamp(height, 0.0, 100.0);
            if (height < target) {
                height += 0.5;
            } else if (height > target) {
                height -= 0.5;
            }
        }
    }
};

std::unique_ptr<common::IController> CreateSuspensionController()
{
    return std::make_unique<SuspensionController>();
}

} // namespace control_vehiculo
