#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace adquisicion_datos {

struct SensorChannel {
    std::string name;
    uint8_t channel{0};
    double scale{1.0};
    double offset{0.0};
};

using SensorConfig = std::vector<SensorChannel>;

} // namespace adquisicion_datos
