#pragma once

#include <vector>
#include <string>
#include <memory>

#include "../common/analog_sample.hpp"
#include "../common/isensor_reader.hpp"

// =============================
// DEFINICIÓN BRAKE SWITCH (IXPIO)
// =============================
#define BRAKE_SW_BIT   0
#define BRAKE_SW_MASK (1u << BRAKE_SW_BIT)

namespace adquisicion_datos {

class Pex1202L : public common::ISensorReader {
public:
    explicit Pex1202L(SensorConfig config);
    ~Pex1202L() override;

    bool start() override;
    void stop() override;

    std::vector<common::AnalogSample> read_samples() override;

private:
    SensorConfig config_;
    bool mock_mode_{false};
};

std::unique_ptr<common::ISensorReader>
CreatePex1202L(SensorConfig config);

} // namespace adquisicion_datos
