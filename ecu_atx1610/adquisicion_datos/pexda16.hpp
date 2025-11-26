#pragma once

#include "../common/interfaces.hpp"

#include <memory>

namespace adquisicion_datos {

std::unique_ptr<common::IActuatorWriter> CreatePexDa16();

} // namespace adquisicion_datos
