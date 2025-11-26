#include "sensor_manager.hpp"
#include "pex1202l.hpp"
#include "sensor_config.hpp"
#include "../common/logging.hpp"

#include <memory>

namespace adquisicion_datos {

class SensorManager::Impl {
public:
    Impl()
    {
        SensorConfig base_config{
            {"acelerador", 0, 1.0, 0.0},
            {"freno", 1, 1.0, 0.0},
            {"volante", 2, 1.0, 0.0},
            {"suspension_fl", 3, 1.0, 0.0},
            {"suspension_fr", 4, 1.0, 0.0},
            {"suspension_rl", 5, 1.0, 0.0},
            {"suspension_rr", 6, 1.0, 0.0},
        };
        reader_ = CreatePex1202L(base_config);
    }

    bool start()
    {
        return reader_->start();
    }

    void stop()
    {
        reader_->stop();
    }

    std::vector<common::AnalogSample> poll()
    {
        return reader_->read_samples();
    }

private:
    std::unique_ptr<common::ISensorReader> reader_;
};

SensorManager::SensorManager() : impl_{std::make_unique<Impl>()} {}
SensorManager::~SensorManager() = default;

bool SensorManager::start() { return impl_->start(); }

void SensorManager::stop() { impl_->stop(); }

std::vector<common::AnalogSample> SensorManager::poll() { return impl_->poll(); }

} // namespace adquisicion_datos
