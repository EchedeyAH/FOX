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
        
        LOG_INFO("PexDa16", "Inicializando PEX-DA16 (Secuencia explícita)...");
        
        ixpci_reg reg;
        
        // 1. Configure Range (Attempt Unipolar 0-10V)
        // ID 291 = IXPCI_AO_CONFIGURATION
        // Value: 0 = 0-10V, 1 = +/-10V (Typical for some ICP cards, guessing 0 for Unipolar)
        reg.id = 291; 
        reg.value = 0; 
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
             LOG_WARN("PexDa16", "Fallo configurando rango AO (posiblemente no soportado)");
        }

        // 2. Enable Output Channels (0, 1, 2, 3 -> Mask 0x0F)
        // ID 225 = IXPCI_ENABLE_DISABLE_DA_CHANNEL
        reg.id = 225;
        reg.value = 0x0F; 
        reg.mode = IXPCI_RM_NORMAL;
        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
             LOG_ERROR("PexDa16", "Fallo habilitando canales AO");
        } else {
             LOG_INFO("PexDa16", "Canales AO habilitados (Mask: 0x0F)");
        }

        // 3. Reset Outputs to 0V
        for (int i = 0; i < 4; ++i) {
             write_output("AO" + std::to_string(i), 0.0);
        }
        
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

            // Mapeo valor (0-5V) a DAC Code.
            // Asumiendo CONFIGURACIÓN UNIPOLAR 0-10V:
            // 0V = 0
            // 10V = 4095
            // 5V = 2047
            
            double v_clamped = std::max(0.0, std::min(value, 5.0));
            uint32_t dac_val = static_cast<uint32_t>((v_clamped / 10.0) * 4095.0);

            // [DEBUG] Log DAC write
            static int dac_log = 0;
            if (dac_log++ % 50 == 0) {
                 LOG_DEBUG("PexDa16", "AO Write: " + channel + " V=" + std::to_string(value) + 
                           " DAC=" + std::to_string(dac_val));
            }

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
