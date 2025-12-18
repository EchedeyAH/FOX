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
#include <cstdint>

// Si el header del driver no lo define, lo calculamos
#ifndef IXPIO_READ_DI
#include <sys/ioctl.h>
#define IXPIO_READ_DI _IOR('i', 1, unsigned int)
#endif

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
            bool ok = false;

            // 1) Intento por ioctl
            if (ioctl(dio_fd, IXPIO_READ_DI, &di_state) >= 0) {
                ok = true;
            } else {
                // 2) Fallback: intentar read() (algunos drivers lo implementan así)
                uint32_t tmp = 0;
                ssize_t n = ::read(dio_fd, &tmp, sizeof(tmp));
                if (n == (ssize_t)sizeof(tmp)) {
                    di_state = tmp;
                    ok = true;
                }

                static int di_err = 0;
                if (di_err++ % 20 == 0) {
                    LOG_WARN("Pex1202L",
                             std::string("Fallo leyendo DI (ioctl IXPIO_READ_DI). errno=") +
                             std::to_string(errno) + " (" + std::string(strerror(errno)) + ")" +
                             " | read()=" + std::to_string((long long)n));
                }
            }

            if (ok) {
                // LOG estado DI (para calcular máscara)
                static int di_log = 0;
                if (di_log++ % 10 == 0) {
                    char buf[64];
                    std::snprintf(buf, sizeof(buf), "DI_STATE=0x%08X", di_state);
                    LOG_INFO("Pex1202L", std::string("Lectura DI OK: ") + buf);
                }

                // Ajusta aquí máscara/polaridad. De momento: activo-alto.
                static constexpr uint32_t kBrakeMask = (1u << 0);
                static constexpr bool kBrakeActiveLow = false;

                bool brake_raw = (di_state & kBrakeMask) != 0;
                if (kBrakeActiveLow) brake_raw = !brake_raw;
                brake_switch_val = brake_raw ? 1.0 : 0.0;
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

            double value = static_cast<double>(raw_val) * channel.scale + channel.offset;

            if ((channel.name == "acelerador" || channel.name == "freno") && channel.scale > 0.5) {
                value = value / 4095.0;
            }

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
