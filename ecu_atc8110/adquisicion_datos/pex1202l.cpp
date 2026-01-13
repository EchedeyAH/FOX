#include "pex1202l.hpp"
#include "../common/logging.hpp"
#include "pex1202_driver.hpp"
#include "pex_device.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace adquisicion_datos {

class Pex1202L : public common::ISensorReader {
public:
    explicit Pex1202L(SensorConfig config) : config_{std::move(config)} {}
    ~Pex1202L() override { stop(); }

    bool start() override {
        int fd = PexDevice::GetInstance().GetFd();
        if (fd < 0) {
            LOG_WARN("Pex1202L", "Iniciando en MODO SIMULACIÓN por falta de hardware.");
            LOG_INFO("Pex1202L", "Control: echo <valor> > /tmp/force_accel");
            mock_mode_ = true;
            return true;
        }
        return true;
    }

    void stop() override {}

    std::vector<common::AnalogSample> read_samples() override {
        std::vector<common::AnalogSample> samples;
        samples.reserve(config_.size() + 1);

        /* --------- Lectura digital (interruptor de freno) ---------- */
        int dio_fd = PexDevice::GetInstance().GetDioFd();
        double brake_switch_val = 0.0;

        if (dio_fd >= 0) {
            uint32_t di_state = 0;

            // Uso de la macro oficial definida en pex1202_driver.hpp
            int rc = ioctl(dio_fd, IXPCI_IOCTL_DI, &di_state);
            if (rc >= 0) {
                // Log del di_state en hex (y cuando cambia) para deducir máscara correcta
                static uint32_t last_di = 0;
                static bool first = true;
                static int log_cnt = 0;

                if (first || di_state != last_di || (log_cnt++ % 20 == 0)) {
                    std::ostringstream oss;
                    oss << "DI ioctl OK. cmd=0x" << std::hex << std::setw(8) << std::setfill('0')
                        << IXPCI_IOCTL_DI
                        << " | di_state=0x" << std::hex << std::setw(8) << std::setfill('0') << di_state;
                    LOG_INFO("Pex1202L", oss.str());
                    first = false;
                    last_di = di_state;
                }

                // Por ahora: activo-alto
                static constexpr uint32_t kBrakeMask = (1u << 0);
                bool brake_raw = (di_state & kBrakeMask) != 0;
                brake_switch_val = brake_raw ? 1.0 : 0.0;
            } else {
                // Throttle warnings para no spamear
                static int warn_cnt = 0;
                if (warn_cnt++ % 20 == 0) {
                    int e = errno;
                    std::ostringstream oss;
                    oss << "Fallo leyendo DI (ioctl IXPIO_READ_DI). "
                        << "cmd=0x" << std::hex << std::setw(8) << std::setfill('0') << IXPCI_IOCTL_DI
                        << " errno=" << std::dec << e << " (" << strerror(e) << ")";
                    LOG_WARN("Pex1202L", oss.str());
                }
            }
        }

        // Override de prueba (si existe /tmp/force_brake)
        FILE* f_brake = fopen("/tmp/force_brake", "r");
        if (f_brake) {
            int force_val = 0;
            if (fscanf(f_brake, "%d", &force_val) == 1 && force_val > 0) {
                brake_switch_val = 1.0;
            }
            fclose(f_brake);
        }
        samples.push_back({"brake_switch", brake_switch_val});

        /* --------- Modo simulación --------- */
        if (mock_mode_) {
            double accel_val = 0.0;
            FILE* f = fopen("/tmp/force_accel", "r");
            if (f) {
                float val = 0.0f;
                if (fscanf(f, "%f", &val) == 1) accel_val = static_cast<double>(val);
                fclose(f);
            }
            for (const auto& channel : config_) {
                double val = 0.0;
                if (channel.name == "acelerador") val = accel_val;
                samples.push_back({channel.name, val});
            }
            return samples;
        }

        /* --------- Lectura analógica (PEX-1202) --------- */
        int fd = PexDevice::GetInstance().GetFd();
        if (fd < 0) return samples;

        bool any_data_read = false;
        for (const auto& channel : config_) {
            ixpci_reg reg;

            // Seleccionar canal
            reg.id = IXPCI_AICR;
            reg.value = channel.channel;
            reg.mode = IXPCI_RM_NORMAL;
            ioctl(fd, IXPCI_WRITE_REG, &reg);

            // Iniciar conversión
            reg.id = IXPCI_ADST;
            reg.value = 0;
            reg.mode = IXPCI_RM_NORMAL;
            ioctl(fd, IXPCI_WRITE_REG, &reg);

            // Leer valor
            reg.id = IXPCI_AI;
            reg.value = 0;
            reg.mode = IXPCI_RM_READY;
            if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
                reg.mode = IXPCI_RM_NORMAL;
                ioctl(fd, IXPCI_READ_REG, &reg);
            }

            uint16_t raw_val = reg.value & 0x0FFF;
            if (raw_val > 0) any_data_read = true;

            // Escala y offset del YAML
            double value = static_cast<double>(raw_val) * channel.scale + channel.offset;

            // Normaliza acelerador y freno a 0-1 si la escala es grande
            if ((channel.name == "acelerador" || channel.name == "freno") && channel.scale > 0.5) {
                value = value / 4095.0;
            }

            // Override de acelerador (modo real)
            if (channel.name == "acelerador") {
                FILE* f_acc = fopen("/tmp/force_accel", "r");
                if (f_acc) {
                    float force_val = 0.0f;
                    if (fscanf(f_acc, "%f", &force_val) == 1) value = static_cast<double>(force_val);
                    fclose(f_acc);
                }
            }

            samples.push_back({channel.name, value});
        }

        // Warning si todas las lecturas son cero
        if (!any_data_read && !samples.empty()) {
            static int zero_count = 0;
            if (++zero_count % 200 == 0) {
                LOG_WARN("Pex1202L", "ALERTA: Todas las lecturas son 0 (Posible desconexión)");
            }
        }

        return samples;
    }

private:
    SensorConfig config_;
    bool mock_mode_{false};
};

std::unique_ptr<common::ISensorReader> CreatePex1202L(SensorConfig config) {
    return std::make_unique<Pex1202L>(std::move(config));
}

} // namespace adquisicion_datos
