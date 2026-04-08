#include "../common/interfaces.hpp"
#include "../common/logging.hpp"
#include "pexda16.hpp"
#include "pex1202_driver.hpp"
#include "pex_device.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <map>
#include <string>
#include <algorithm>

// Incluir driver PEX-DA16 (ixpio)
extern "C" {
    #include "ixpio.h"
}

namespace adquisicion_datos {

class PexDa16 : public common::IActuatorWriter {
public:
    PexDa16() = default;

    bool start() override
    {
        // PEX-DA16 usa ixpio (no ixpci)
        dev_path_ = "/dev/ixpio1";
        fd_ = open(dev_path_.c_str(), O_RDWR);
        if (fd_ < 0) {
            LOG_WARN("PexDa16", "No se pudo abrir " + dev_path_ +
                               ". Probando fallback a /dev/ixpio0...");
            dev_path_ = "/dev/ixpio0";
            fd_ = open(dev_path_.c_str(), O_RDWR);
        }

        if (fd_ < 0) {
            LOG_ERROR("PexDa16", "No se encontró hardware PEX-DA16 en /dev/ixpio1 o /dev/ixpio0.");
            return false;
        }

        LOG_INFO("PexDa16", "Inicializando PEX-DA16 en " + dev_path_ + ", FD: " + std::to_string(fd_));

        bool ok = true;

        // Poner AO0..AO3 a 0V (test seguro)
        for (int i = 0; i < 4; ++i) {
            if (!write_ao_voltage(i, 0.0)) {
                ok = false;
            }
        }

        if (!ok) {
            LOG_ERROR("PexDa16", "Fallo inicializando PEX-DA16 (AO no operativo)");
            return false;
        }

        LOG_INFO("PexDa16", "AO test write OK");
        return true;
    }

    void stop() override {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

    void write_output(const std::string &channel, double value) override
    {
        outputs_[channel] = value;

        if (fd_ < 0) return;

        // ENABLE no soportado en ixpio (evitar ioctl incorrecto)
        if (channel == "ENABLE") {
            LOG_ERROR("PexDa16", "ENABLE no soportado en ixpio (PEX-DA16)");
            return;
        }

        // AO0..AO15
        if (channel.rfind("AO", 0) == 0 && channel.length() > 2) {
            int ch = 0;
            try {
                ch = std::stoi(channel.substr(2));
            } catch (...) {
                LOG_ERROR("PexDa16", "Canal inválido: " + channel);
                return;
            }

            write_ao_voltage(ch, value);
        }
    }

    const std::map<std::string, double> &outputs() const { return outputs_; }

private:
    static inline uint16_t voltageToRaw(double voltage)
    {
        if (voltage > 10.0) voltage = 10.0;
        if (voltage < -10.0) voltage = -10.0;
        return (uint16_t)((voltage + 10.0) * (16383.0 / 20.0));
    }

    bool write_ao_voltage(int ch, double value) {
        double v_used = value;
        if (ch == 0) v_used = 5.0;
        uint16_t raw = voltageToRaw(v_used);

        ixpio_analog_t ao;
        memset(&ao, 0, sizeof(ao));
        ao.channel = ch;
        ao.data.u16 = raw;

        if (ioctl(fd_, IXPIO_ANALOG_OUT, &ao) != 0) {
            perror("PEX-DA16 write failed");
            return false;
        }

        printf("AO DEBUG -> ch=%d voltage=%.2f raw=0x%04x\n", ch, v_used, raw);
        return true;
    }

    std::map<std::string, double> outputs_;
    int fd_ = -1;
    std::string dev_path_;
};

std::unique_ptr<common::IActuatorWriter> CreatePexDa16()
{
    return std::make_unique<PexDa16>();
}

} // namespace adquisicion_datos
