#include "pex1202l.hpp"
#include "../common/logging.hpp"

#include <random>

namespace adquisicion_datos {

class Pex1202L : public common::ISensorReader {
public:
    explicit Pex1202L(SensorConfig config) : config_{std::move(config)} {}

    bool start() override
    {
        LOG_INFO("Pex1202L", "Inicializando lectura analógica");
        return true;
    }

    void stop() override
    {
        LOG_INFO("Pex1202L", "Deteniendo lectura analógica");
    }

    std::vector<common::AnalogSample> read_samples() override
    {
        std::vector<common::AnalogSample> samples;
        samples.reserve(config_.size());
        for (const auto &channel : config_) {
            const double raw = generate_signal();
            const double value = raw * channel.scale + channel.offset;
            samples.push_back({channel.name, value});
        }
        return samples;
    }

private:
    SensorConfig config_;
    std::default_random_engine rng_{std::random_device{}()};
    std::normal_distribution<double> noise_{0.0, 0.01};

    double generate_signal()
    {
        return 1.0 + noise_(rng_);
    }
};

std::unique_ptr<common::ISensorReader> CreatePex1202L(SensorConfig config)
{
    return std::make_unique<Pex1202L>(std::move(config));
}

} // namespace adquisicion_datos
