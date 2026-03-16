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

// Incluir driver PEX
extern "C" {
    #include "ixpci.h"
}

// Definición para Digital Output Port si no está en el driver header
#ifndef IXPCI_DO
#define IXPCI_DO 212
#endif

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

        ixpci_reg_t reg;
        memset(&reg, 0, sizeof(reg));
        bool ok = true;

        // 1) Configurar rango AO (si el driver lo soporta)
        // ID 291 = IXPCI_AO_CONFIGURATION (según tu comentario)
        reg.id = 291;
        reg.value = 0; // 0 = 0-10V (intento)
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
            LOG_WARN("PexDa16", std::string("Fallo configurando rango AO: errno=") +
                                std::to_string(errno) + " (" + std::string(strerror(errno)) + ")");
            ok = false;
        }

        // 2) Habilitar canales AO (0..3)
        // ID 225 = IXPCI_ENABLE_DISABLE_DA_CHANNEL (según tu comentario)
        reg.id = 225;
        reg.value = 0x0F;
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
            LOG_ERROR("PexDa16", std::string("Fallo habilitando canales AO: errno=") +
                                 std::to_string(errno) + " (" + std::string(strerror(errno)) + ")");
            ok = false;
        } else {
            LOG_INFO("PexDa16", "Canales AO habilitados (Mask: 0x0F)");
        }

        // 3) Poner AO0..AO3 a 0V (test seguro)
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

        // ENABLE por salida digital (bit0) en IXPCI_DO (según tu implementación)
        if (channel == "ENABLE") {
            bool enable = (value > 0.5);

            ixpci_reg_t reg;
            memset(&reg, 0, sizeof(reg));
            
            reg.id = IXPCI_DO;
            reg.value = enable ? 1 : 0;   // bit 0
            reg.mode = IXPCI_RM_NORMAL;

            if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
                static int err_count = 0;
                if (err_count++ % 50 == 0) {
                    LOG_ERROR("PexDa16",
                              std::string("Error escribiendo ENABLE (IXPCI_DO). errno=") +
                              std::to_string(errno) + " (" + std::string(strerror(errno)) + ")");
                }
            }
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
    bool write_ao_voltage(int ch, double value) {
        // Clamp a 0..5V en tu lógica (si tu mando real es 0..10V, luego lo ajustamos)
        double v_clamped = std::clamp(value, 0.0, 5.0);

        // Si el rango real es 0..10V y DAC 12-bit: code = V/10 * 4095
        uint32_t dac_val = static_cast<uint32_t>((v_clamped / 10.0) * 4095.0);
        if (dac_val > 4095) dac_val = 4095;

        static int dac_log = 0;
        if (dac_log++ % 50 == 0) {
            LOG_DEBUG("PexDa16", "AO Write: AO" + std::to_string(ch) +
                                " V=" + std::to_string(value) +
                                " DAC=" + std::to_string(dac_val));
        }

        ixpci_reg_t reg;
        memset(&reg, 0, sizeof(reg));

        if (ch == 0) reg.id = 222;       // IXPCI_AO0
        else if (ch == 1) reg.id = 223;  // IXPCI_AO1
        else if (ch == 2) reg.id = 224;  // IXPCI_AO2
        else {
            // Fallback para AO3+ si tu driver usa “canal<<12 | dac”
            reg.id = 220;
            reg.value = (static_cast<uint32_t>(ch) << 12) | dac_val;
            reg.mode = IXPCI_RM_NORMAL;
            if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
                static int ao_err = 0;
                if (ao_err++ % 50 == 0) {
                    LOG_ERROR("PexDa16",
                              std::string("Error writing AO") + std::to_string(ch) +
                              " errno=" + std::to_string(errno) +
                              " (" + std::string(strerror(errno)) + ")");
                }
                return false;
            }
            return true;
        }

        reg.value = dac_val;
        reg.mode = IXPCI_RM_NORMAL;

        if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
            static int ao_err = 0;
            if (ao_err++ % 50 == 0) {
                LOG_ERROR("PexDa16",
                          std::string("Error writing AO") + std::to_string(ch) +
                          " errno=" + std::to_string(errno) +
                          " (" + std::string(strerror(errno)) + ")");
            }
            return false;
        }
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
