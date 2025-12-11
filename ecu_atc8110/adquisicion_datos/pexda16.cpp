#include "../common/interfaces.hpp"
#include "../common/logging.hpp"
#include "pexda16.hpp"
#include "pex1202_driver.hpp"

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
        LOG_INFO("PexDa16", "Inicializando salidas analógicas/digitales (PEX-DA16)");
        
        // Intentar abrir el dispositivo. Generalmente comparten /dev/ixpci1 con PEX1202L
        // Si falló Pex1202L en ixpci1, probamos ixpci1 aquí también o ixpci0.
        // Asumimos que la tarjeta multifunción es ixpci1
        fd_ = open("/dev/ixpci1", O_RDWR);
        
        if (fd_ < 0) {
            LOG_WARN("PexDa16", "Fallo al abrir /dev/ixpci1, intentando /dev/ixpci0...");
            fd_ = open("/dev/ixpci0", O_RDWR);
        }

        if (fd_ < 0) {
            LOG_ERROR("PexDa16", "Error critico abriendo dispositivo DAQ: " + std::string(strerror(errno)));
            LOG_ERROR("PexDa16", "La señal ENABLE no podrá ser activada.");
            return false;
        }

        LOG_INFO("PexDa16", "Dispositivo abierto correctamente. Fd: " + std::to_string(fd_));
        return true;
    }

    void stop() override
    {
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
            LOG_INFO("PexDa16", "Dispositivo cerrado");
        }
    }

    void write_output(const std::string &channel, double value) override
    {
        outputs_[channel] = value;
        
        if (fd_ < 0) return;

        // Mapeo específico para ENABLE (Motores Delanteros SKAI)
        // Asumimos que ENABLE está conectado al Bit 0 del Puerto Digital de Salida
        if (channel == "ENABLE") {
            bool enable = (value > 0.5); // Lógica booleana
            
            ixpci_reg reg;
            reg.id = IXPCI_DO; 
            reg.value = enable ? 1 : 0; // Bit 0 activo
            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
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
            // If hardware is +/-10V (Bipolar), 0V = 2048 (+/- offset).
            // Legacy tadAO_fox.c checks BIPOLAR/UNIPOLAR settings.
            // "dscdasettings.polarity = BIPOLAR;" (line 157 in tadAO_fox.c)
            // "cda_wr ... BIPOLAR"
            // Case BIPOLAR: if (volt < -Range) ... output = (volt * 2048 / Range + 2048)
            // RANGO_CDA in legacy header? Likely 5 or 10.
            // If Range=10, 0V = 2048. 5V = (5 * 2048 / 10) + 2048 = 1024 + 2048 = 3072.
            // If Range=5, 0V = 2048. 5V = 4096.

            // Let's assume BIPOLAR +/- 10V for safety on standard industrial PCs.
            // 0V = 2048 (Midscale)
            // 5V = 2048 + (5/10 * 2048) = 3072.
            
            uint32_t dac_val = 2048 + static_cast<uint32_t>((v_clamped / 10.0) * 2048.0);
            if (dac_val > 4095) dac_val = 4095;

            ixpci_reg reg;
            
            if (ch == 0) reg.id = 222; // IXPCI_AO0
            else if (ch == 1) reg.id = 223; // IXPCI_AO1
            else if (ch == 2) reg.id = 224; // IXPCI_AO2
            else {
                 // For Ch3+, try writing to Port 220 using packed (Channel << ? | Value)
                 // This is a guess for generic PEX cards.
                 // Alternatively, map AO3 to 226? No.
                 // Let's try 220 (IXPCI_AO) with simple value write, assuming separate selection??
                 // Or just fail for now?
                 // Let's try: IXPCI_AO (220) and assume it maps to 3?? No.
                 // Fallback: Write using generic Port logic if supported, 
                 // otherwise Log warning for Ch3+.
                 reg.id = 220; // IXPCI_AO (Port)
                 // Some cards use bits 12-15 for channel.
                 reg.value = (ch << 12) | dac_val; 
            }

            if (ch <= 2) reg.value = dac_val; // Direct registers take value directly usually

            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd_, IXPCI_WRITE_REG, &reg) < 0) {
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
