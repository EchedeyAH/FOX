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
        // Mapeo de canales exacto del código C funcional
        // Ver read_channels.c para referencia de pines físicos
        SensorConfig base_config{
            // Canal AI_1 (pin 2) - Acelerador - Unipolar 0-5V
            {"acelerador", 1, 1.0, 0.0},
            
            // Canal AI_3 (pin 4) - Freno - Unipolar 0-5V
            {"freno", 3, 1.0, 0.0},
            
            // Canal AI_4 (pin 5) - Volante - Unipolar 0-5V
            {"volante", 4, 0.9280, -3.8254}, // Calibración legacy
            
            // Canal AI_2 (pin 3) - Corriente Eje Delantero - Bipolar ±5V
            {"corriente_eje_d", 2, 1.0, 0.0},
            
            // Canal AI_5 (pin 6) - Corriente Eje Trasero - Bipolar ±5V
            {"corriente_eje_t", 5, 1.0, 0.0},
            
            // Canal AI_10 (pin 11) - Suspensión Delantera Izquierda - Unipolar 0-5V
            {"suspension_fl", 10, 1.0, 0.0},
            
            // Canal AI_12 (pin 13) - Suspensión Delantera Derecha - Unipolar 0-5V
            {"suspension_fr", 12, 1.0, 0.0},
            
            // Canal AI_6 (pin 7) - Suspensión Trasera Izquierda - Unipolar 0-5V
            {"suspension_rl", 6, 1.0, 0.0},
            
            // Canal AI_8 (pin 9) - Suspensión Trasera Derecha - Unipolar 0-5V
            {"suspension_rr", 8, 1.0, 0.0},
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
