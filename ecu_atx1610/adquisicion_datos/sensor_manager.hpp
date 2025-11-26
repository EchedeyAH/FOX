#pragma once

#include "../common/types.hpp"

#include <memory>
#include <vector>

namespace adquisicion_datos {

class SensorManager {
public:
    SensorManager();
    ~SensorManager();
    bool start();
    void stop();
    std::vector<common::AnalogSample> poll();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace adquisicion_datos
