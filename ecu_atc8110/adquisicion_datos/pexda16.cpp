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

#ifndef IXPCI_DO
#define IXPCI_DO 212
#endif

namespace adquisicion_datos {

class PexDa16 : public common::IActuatorWriter {
public:
    PexDa16() = default;

    bool start() override {
        analog_fd_ = PexDevice::GetInstance().GetFd();   // /dev/ixpciX para AO
        dio_fd_    = PexDevice::GetInstance().GetDioFd(); // /dev/ixpioX para digital

        if (dio_fd_ < 0) {
            LOG_ERROR("PexDa16", "La señal ENABLE no podrá ser activada (FD digital inválido).");
            return false;
        }
        if (analog_fd_ < 0) {
            LOG_ERROR("PexDa16", "Salidas analógicas no disponibles (FD analógico inválido).");
            return false;
        }

        LOG_INFO("PexDa16", "Inicializando PEX‑DA16 (Secuencia explícita)...");
        ixpci_reg reg;

        // Configurar AO a 0-10 V unipolar
        reg.id = 291; // IXPCI_AO_CONFIGURATION
        reg.value = 0; // 0 = 0‑10 V
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(analog_fd_, IXPCI_WRITE_REG, &reg) < 0) {
            LOG_WARN("PexDa16", "Fallo configurando rango AO (posiblemente no soportado)");
        }

        // Habilitar canales AO 0-3
        reg.id = 225;    // IXPCI_ENABLE_DISABLE_DA_CHANNEL
        reg.value = 0x0F;
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(analog_fd_, IXPCI_WRITE_REG, &reg) < 0) {
            LOG_ERROR("PexDa16", "Fallo habilitando canales AO");
        } else {
            LOG_INFO("PexDa16", "Canales AO habilitados (Mask: 0x0F)");
        }

        // Poner salidas a 0 V
        for (int i = 0; i < 4; ++i) {
            write_output("AO" + std::to_string(i), 0.0);
        }
        return true;
    }

    void stop() override {}

    void write_output(const std::string& channel, double value) override {
        outputs_[channel] = value;

        // Señal digital ENABLE
        if (channel == "ENABLE") {
            if (dio_fd_ < 0) return;
            bool enable = (value > 0.5);
            ixpci_reg reg;
            reg.id = IXPCI_DO;
            reg.value = enable ? 1 : 0;
            reg.mode = IXPCI_RM_NORMAL;
            ioctl(dio_fd_, IXPCI_WRITE_REG, &reg);
            return;
        }

        // Salidas analógicas AO0-AO3
        if (channel.rfind("AO", 0) == 0 && channel.length() > 2) {
            if (analog_fd_ < 0) return;
            int ch = 0;
            try { ch = std::stoi(channel.substr(2)); } catch (...) {
                LOG_ERROR("PexDa16", "Canal inválido: " + channel);
                return;
            }

            // Convertir 0‑5 V (pedales) a 0‑10 V DAC en rango 0‑4095
            double v_clamped = std::max(0.0, std::min(value, 5.0));
            uint32_t dac_val = static_cast<uint32_t>((v_clamped / 10.0) * 4095.0);
            if (dac_val > 4095) dac_val = 4095;

            ixpci_reg reg;
            if (ch == 0) { reg.id = 222; reg.value = dac_val; }
            else if (ch == 1) { reg.id = 223; reg.value = dac_val; }
            else if (ch == 2) { reg.id = 224; reg.value = dac_val; }
            else {
                reg.id = 220;
                reg.value = (ch << 12) | dac_val;
            }
            reg.mode = IXPCI_RM_NORMAL;
            if (ioctl(analog_fd_, IXPCI_WRITE_REG, &reg) < 0) {
                static int ao_err = 0;
                if (ao_err++ % 50 == 0) {
                    LOG_ERROR("PexDa16", "Error writing AO" + std::to_string(ch));
                }
            }
        }
    }

    const std::map<std::string, double>& outputs() const { return outputs_; }

private:
    int dio_fd_    = -1;
    int analog_fd_ = -1;
    std::map<std::string, double> outputs_;
};

std::unique_ptr<common::IActuatorWriter> CreatePexDa16() {
    return std::make_unique<PexDa16>();
}

} // namespace adquisicion_datos
