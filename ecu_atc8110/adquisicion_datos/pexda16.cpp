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
        // Get shared FD
        int fd = PexDevice::GetInstance().GetFd();
        if (fd < 0) {
            LOG_ERROR("PexDa16", "La señal ENABLE no podrá ser activada (FD Invalido).");
            return false;
        }
        LOG_INFO("PexDa16", "Inicializando salidas analógicas/digitales (PEX-DA16)");
        return true;
    }

    void stop() override
    {
        // PexDevice::GetInstance().Close();
    }

    void write_output(const std::string &channel, double value) override
    {
        outputs_[channel] = value;
        int fd = PexDevice::GetInstance().GetFd();
        
        if (fd < 0) return;

        // Mapeo específico para ENABLE (Motores Delanteros SKAI)
        // Asumimos que ENABLE está conectado al Bit 0 del Puerto Digital de Salida
        if (channel == "ENABLE") {
            bool enable = (value > 0.5); // Lógica booleana
            
            ixpci_reg reg;
            reg.id = IXPCI_DO; 
            reg.value = enable ? 1 : 0; // Bit 0 activo
            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
                // Throttle logs
                static int err_count = 0;
                if (err_count++ % 50 == 0) {
                     LOG_ERROR("PexDa16", "Error escribiendo salida digital ENABLE");
                }
            }
            return;
        }

        // Mapeo para Salidas Analógicas (AO0 - AO15)
        // Formato esperado: "AO0", "AO1", ... "AO3"
        if (channel.rfind("AO", 0) == 0 && channel.length() > 2) {
            int ch = 0;
            try {
                ch = std::stoi(channel.substr(2));
            } catch (...) {
                LOG_ERROR("PexDa16", "Canal invalido: " + channel);
                return;
            }

            // Mapeo de valor (0-5V) a DAC Code (0-4095)
            // Asumimos rango unipolar 0-10V o 0-5V hardware
            // Legacy code: 10V Range. 0-5V output -> 0 - 2048 (aprox)
            // Si el rango es 10V (común): 5V = 2047.
            // Si el rango es 5V: 5V = 4095.
            // Asumiremos Rango 10V para seguridad (para no sobrevoltar 5V inputs)
            // TODO: Verificar RANGO exacto. Legacy dice "Entrada Throttle 0-5V".
            
            // Limit to 0-5V logic
            double v_clamped = std::max(0.0, std::min(value, 5.0));
            
            // Scaled to 12-bit (Assuming 10V range, so 5V is half scale)
            // 4095 = 10V => 5V = 2047 
            // 0V = 2048 (Midscale)
            // 5V = 2048 + (5/10 * 2048) = 3072.
            
            uint32_t dac_val = 2048 + static_cast<uint32_t>((v_clamped / 10.0) * 2048.0);
            if (dac_val > 4095) dac_val = 4095;

            ixpci_reg reg;
            
            if (ch == 0) reg.id = 222; // IXPCI_AO0
            else if (ch == 1) reg.id = 223; // IXPCI_AO1
            else if (ch == 2) reg.id = 224; // IXPCI_AO2
            else {
                 // Fallback for AO3
                 reg.id = 220; 
                 reg.value = (ch << 12) | dac_val; 
            }

            if (ch <= 2) reg.value = dac_val;

            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
                 static int ao_err = 0;
                 if (ao_err++ % 50 == 0) LOG_ERROR("PexDa16", "Error writing AO" + std::to_string(ch));
            }
        }
    }

    const std::map<std::string, double> &outputs() const { return outputs_; }

private:
    int fd_ = -1;
    std::map<std::string, double> outputs_;
};

std::unique_ptr<common::IActuatorWriter> CreatePexDa16()
{
    return std::make_unique<PexDa16>();
}

} // namespace adquisicion_datos
