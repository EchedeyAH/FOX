#include "pex1202l.hpp"
#include <ixpio.h>

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
        int fd = PexDevice::GetInstance().GetFd(1); // Tarjeta 1 (Sensores / PEX-1202)
        if (fd < 0) {
            LOG_WARN("Pex1202L", "Iniciando en MODO SIMULACIÓN por falta de hardware en /dev/ixpci1.");
            LOG_INFO("Pex1202L", "Control: echo <valor> > /tmp/force_accel");
            mock_mode_ = true;
            return true;
        }

        // Configurar rango +/-5V (si el driver lo soporta)
        ixpci_reg reg;
        reg.id = IXPCI_ADGCR;
        reg.value = PEX_GAIN_BIP_5V; // +/- 5V
        reg.mode = IXPCI_RM_NORMAL;

        int rc = ioctl(fd, IXPCI_WRITE_REG, &reg);
        if (rc < 0) {
            LOG_ERROR("Pex1202L", "Fallo configurando Gain +/-5V (ioctl IXPCI_WRITE_REG ADGCR)");
            // no return false: seguimos
        } else {
            LOG_INFO("Pex1202L", "Configurado Rango +/- 5V (Gain Code: 0x00)");
        }

        return true;
    }

    void stop() override {}

    std::vector<common::AnalogSample> read_samples() override {
        std::vector<common::AnalogSample> samples;
        samples.reserve(config_.size() + 1);

        /* --------- Lectura digital (interruptor de freno) ---------- */
        int dio_fd = PexDevice::GetInstance().GetDioFd(1); // Tarjeta 1
        double brake_switch_val = 0.0;

        if (dio_fd >= 0) {
            ixpio_digital_t di;
            std::memset(&di, 0, sizeof(di));

            int rc = ioctl(dio_fd, IXPIO_DIGITAL_IN, &di);
            if (rc >= 0) {
                uint16_t di_state = di.data.u16; // <-- bitmap DI (típico 16 bits)
                int rc = ioctl(dio_fd, IXPIO_DIGITAL_IN, &di);
                if (rc >= 0) {
                    uint16_t di_state = di.data.u16;

                    // ===== DEBUG CRÍTICO: ver TODOS los bits =====
                    std::ostringstream oss;
                    oss << "IXPIO_DI RAW = 0x"
                        << std::hex << std::setw(4) << std::setfill('0') << di_state
                        << " | bits activos: ";

                    for (int i = 0; i < 16; ++i) {
                        if (di_state & (1u << i)) {
                            oss << i << " ";
                        }
                    }

                    LOG_INFO("Pex1202L", oss.str());
                    // ============================================

                    // ❌ NO apliques aún ninguna máscara fija
                    brake_switch_val = 0.0;   // lo forzamos a 0 mientras diagnosticamos
                }

                // Override de prueba (si existe /tmp/force_brake)
                FILE* f_brake = fopen("/tmp/force_brake", "r");
                if (f_brake) {
                    int force_val = 0;
                    if (fscanf(f_brake, "%d", &force_val) == 1 && force_val > 0) {
                        di_state = (1u << 0); // Forzamos DI0
                    }
                    fclose(f_brake);
                }

            // Log cuando cambie (o cada N)
            static uint16_t last_di = 0;
            static bool first = true;
            static int log_cnt = 0;

            if (first || di_state != last_di || (log_cnt++ % 20 == 0)) {
                std::ostringstream oss;
                oss << "IXPIO_DIGITAL_IN OK | di_state=0x"
                    << std::hex << std::setw(4) << std::setfill('0') << di_state;
                LOG_INFO("Pex1202L", oss.str());
                first = false;
                last_di = di_state;
            }

            // Por ahora: DI0 como freno (luego lo descubrimos si no es)
            static constexpr uint16_t kBrakeMask = (1u << 0);
            bool brake_raw = (di_state & kBrakeMask) != 0;
            brake_switch_val = brake_raw ? 1.0 : 0.0;
        } else {
            static int warn_cnt = 0;
            if (warn_cnt++ % 20 == 0) {
                int e = errno;
                std::ostringstream oss;
                oss << "Fallo leyendo DI (ioctl IXPIO_DIGITAL_IN). "
                    << "errno=" << e << " (" << strerror(e) << ")";
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
        int fd = PexDevice::GetInstance().GetFd(1);
        if (fd < 0) return samples;

        bool any_data_read = false;

        for (const auto& channel : config_) {
            ixpci_reg reg;

            // Seleccionar canal
            reg.id = IXPCI_AICR;
            reg.value = channel.channel;
            reg.mode = IXPCI_RM_NORMAL;
            (void)ioctl(fd, IXPCI_WRITE_REG, &reg);

            // Iniciar conversión
            reg.id = IXPCI_ADST;
            reg.value = 0;
            reg.mode = IXPCI_RM_NORMAL;
            (void)ioctl(fd, IXPCI_WRITE_REG, &reg);

            // Leer valor
            reg.id = IXPCI_AI;
            reg.value = 0;
            reg.mode = IXPCI_RM_READY;
            if (ioctl(fd, IXPCI_READ_REG, &reg) < 0) {
                reg.mode = IXPCI_RM_NORMAL;
                (void)ioctl(fd, IXPCI_READ_REG, &reg);
            }

            const uint16_t raw_val = static_cast<uint16_t>(reg.value & 0x0FFF);
            if (raw_val > 0) any_data_read = true;

            // Conversión aproximada para +/-5V:
            // V = (raw - 2048) * (5.0 / 2048.0)
            double value_volts = (static_cast<double>(raw_val) - 2048.0) * (5.0 / 2048.0);
            if (value_volts < 0.0) value_volts = 0.0;

            double value = 0.0;
            if (channel.name == "acelerador" || channel.name == "freno") {
                value = value_volts / 5.0;
                if (value > 1.0) value = 1.0;
            } else {
                value = value_volts * channel.scale + channel.offset;
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
