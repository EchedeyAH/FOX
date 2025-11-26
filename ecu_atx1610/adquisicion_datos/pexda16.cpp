#include "../common/interfaces.hpp"
#include "../common/logging.hpp"

#include <map>
#include <string>

namespace adquisicion_datos {

class PexDa16 : public common::IActuatorWriter {
public:
    bool start() override
    {
        LOG_INFO("PexDa16", "Inicializando salidas analógicas/digitales");
        return true;
    }

    void stop() override
    {
        LOG_INFO("PexDa16", "Desactivando salidas");
    }

    void write_output(const std::string &channel, double value) override
    {
        outputs_[channel] = value;
        LOG_DEBUG("PexDa16", "Canal " + channel + " = " + std::to_string(value));
    }

    const std::map<std::string, double> &outputs() const { return outputs_; }

private:
    std::map<std::string, double> outputs_;
};

} // namespace adquisicion_datos
