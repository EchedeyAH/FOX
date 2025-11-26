#pragma once

#include "../common/interfaces.hpp"

#include <memory>

namespace control_vehiculo {

std::unique_ptr<common::IController> CreateSuspensionController();
std::unique_ptr<common::IController> CreateBatteryManager();
std::unique_ptr<common::IController> CreateTractionControl();

} // namespace control_vehiculo
