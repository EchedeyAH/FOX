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
                // Solo loguear error ocasionalmente para no saturar
                static int err_count = 0;
                if (err_count++ % 50 == 0) {
                     LOG_ERROR("PexDa16", "Error escribiendo salida digital ENABLE");
                }
            } else {
                // LOG_DEBUG("PexDa16", "ENABLE set to " + std::to_string(reg.value));
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
