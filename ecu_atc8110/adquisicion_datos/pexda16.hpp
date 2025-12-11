#pragma once

#include "../common/interfaces.hpp"
#include <memory>

namespace adquisicion_datos {

// Factory function to create PexDa16 instance
std::unique_ptr<common::IActuatorWriter> CreatePexDa16();

} // namespace adquisicion_datos
