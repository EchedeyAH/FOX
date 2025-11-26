#include "pexda16.hpp"
#include "../common/logging.hpp"

#include <map>
#include <string>

namespace adquisicion_datos {

class PexDa16 : public common::IActuatorWriter {
public:
    bool start() override
    {
        LOG_INFO("PexDa16", "Inicializando salidas analógicas/digitales");
        ready_ = true;
        return true;
    }

    void stop() override
    {
        ready_ = false;
        LOG_INFO("PexDa16", "Desactivando salidas");
    }

    void write_output(const std::string &channel, double value) override
    {
        if (!ready_) {
            LOG_WARN("PexDa16", "Ignorando escritura en canal sin iniciar");
            return;
        }
        outputs_[channel] = value;
        LOG_DEBUG("PexDa16", "Canal " + channel + " = " + std::to_string(value));
    }

    const std::map<std::string, double> &outputs() const { return outputs_; }

private:
    std::map<std::string, double> outputs_;
    bool ready_{false};
};

std::unique_ptr<common::IActuatorWriter> CreatePexDa16()
{
    return std::make_unique<PexDa16>();
}

} // namespace adquisicion_datos
