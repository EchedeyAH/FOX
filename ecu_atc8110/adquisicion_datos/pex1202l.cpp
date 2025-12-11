#include "pex1202l.hpp"
#include "../common/logging.hpp"
#include "pex1202_driver.hpp"
#include "pex_device.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>

namespace adquisicion_datos {

class Pex1202L : public common::ISensorReader {
public:
    explicit Pex1202L(SensorConfig config) : config_{std::move(config)} {}

    ~Pex1202L() override {
        stop();
    }

    bool start() override
    {
        // Get shared FD
        int fd = PexDevice::GetInstance().GetFd();
        return (fd >= 0);
    }

    void stop() override
    {
        // Singleton manages lifecycle, but we can explicitly close if needed (not recommended if shared)
        // PexDevice::GetInstance().Close(); 
    }

    std::vector<common::AnalogSample> read_samples() override
    {
        std::vector<common::AnalogSample> samples;
        int fd = PexDevice::GetInstance().GetFd();
        if (fd < 0) return samples;

        samples.reserve(config_.size());
        bool any_data_read = false;

        for (const auto &channel : config_) {
            // 1. Configurar canal (Multiplexer)
            // Nota: Para PEX-1202L, el canal se selecciona en IXPCI_AICR o IXPCI_ADGCR
            // Asumimos canal simple (Single Ended) 0-31
            
            ixpci_reg reg;
            reg.id = IXPCI_AICR; // Channel Control
            reg.value = channel.channel; // 0 to N
            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
                // Ignore Operation not permitted errors if they are spurious, or log debug
                if (errno != EPERM) {
                     std::cerr << "[ERROR] [Pex1202L] Error seleccionando canal " << channel.channel << ": " << strerror(errno) << std::endl;
                }
                // continue; // Try to continue anyway?
            }

            // 2. Disparar conversión (Software Trigger)
            reg.id = IXPCI_ADST;
            reg.value = 0; // Valor dummy para trigger
            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
                 if (errno != EPERM) std::cerr << "[ERROR] [Pex1202L] Error disparando trigger" << std::endl;
                // continue;
            }

            // 3. Esperar conversión (Polling)
            
            reg.id = IXPCI_AI; // Data Port
            reg.value = 0;
            reg.mode = IXPCI_RM_READY; // Bloquear hasta listo
            
            if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
                 reg.mode = IXPCI_RM_NORMAL;
                 if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
                    // std::cerr << "[ERROR] [Pex1202L] Error leyendo datos canal " << channel.channel << std::endl;
                    // continue;
                 }
            }

            // 4. Procesar valor (12-bit: 0-4095)
            uint16_t raw_val = reg.value & 0x0FFF;
            
            // Convertir a voltaje (Asumiendo rango +/- 10V o 0-10V segun config hardware)
             // ...
            // Por defecto PEX-1202L suele ser +/- 5V o +/- 10V.
            // Mapeo simple: 0 = -10V, 4095 = +10V (Bipolar) o 0=0V, 4095=10V (Unipolar)
            // Usaremos el factor de escala de la configuración para ajustar.
            
            // Si detectamos que todo es 0, podría ser un error de conexión
            if (raw_val > 0) any_data_read = true;

            double value = static_cast<double>(raw_val) * channel.scale + channel.offset;
            samples.push_back({channel.name, value});
        }

        if (!any_data_read && !samples.empty()) {
            static int zero_count = 0;
            zero_count++;
            if (zero_count % 100 == 0) { // Log cada 100 ciclos para no saturar
                LOG_WARN("Pex1202L", "ALERTA: Todas las lecturas son 0. Verificar conexiones.");
            }
        }

        return samples;
    }

private:
    SensorConfig config_;
    int fd_{-1};
};

std::unique_ptr<common::ISensorReader> CreatePex1202L(SensorConfig config)
{
    return std::make_unique<Pex1202L>(std::move(config));
}

} // namespace adquisicion_datos
