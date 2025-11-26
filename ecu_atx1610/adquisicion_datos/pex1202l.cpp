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
            const double raw = generate_signal(channel.name);
            const double value = raw * channel.scale + channel.offset;
            samples.push_back({channel.name, value});
        }
        return samples;
    }

private:
    SensorConfig config_;
    std::default_random_engine rng_{std::random_device{}()};
    std::normal_distribution<double> noise_{0.0, 0.01};

    double generate_signal(const std::string &name)
    {
        const double base = name == "acelerador"   ? 0.35
                            : name == "freno"      ? 0.10
                            : name == "volante"    ? 0.50
                            : name.find("suspension") != std::string::npos ? 40.0
                                                                              : 1.0;
        return base + noise_(rng_);
    }
};

std::unique_ptr<common::ISensorReader> CreatePex1202L(SensorConfig config)
{
    return std::make_unique<Pex1202L>(std::move(config));
}

} // namespace adquisicion_datos
