#pragma once

#include "sensor_config.hpp"
#include "../common/interfaces.hpp"

#include <memory>

namespace adquisicion_datos {

std::unique_ptr<common::ISensorReader> CreatePex1202L(SensorConfig config);

} // namespace adquisicion_datos
