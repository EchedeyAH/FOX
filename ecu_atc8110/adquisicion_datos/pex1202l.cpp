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
#include <string>
#include <algorithm>

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

            // OJO: este ioctl es “magic number” en tu código actual.
            // Si el fabricante provee un header/define real, mejor usarlo.
            const unsigned long IXPIO_READ_DI = 0x40046901;

            if (ioctl(dio_fd, IXPIO_READ_DI, &di_state) >= 0) {

                // 1) LOG básico de di_state (throttle para no spamear)
                static int di_log = 0;
                if (di_log++ % 20 == 0) { // ~1s si tu loop es 50ms
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "DI State (raw): 0x%08X", di_state);
                    LOG_INFO("Pex1202L", std::string(buf));
                }

                // 2) Captura "reposo" y calcula bits que cambian (XOR)
                //    - baseline: primer valor observado (ajustable si quieres)
                static bool baseline_set = false;
                static uint32_t baseline_di = 0;
                static uint32_t last_di = 0;

                if (!baseline_set) {
                    baseline_di = di_state;
                    last_di = di_state;
                    baseline_set = true;

                    char buf[160];
                    std::snprintf(buf, sizeof(buf),
                                  "Brake DI baseline set: 0x%08X (pisar freno para detectar máscara)",
                                  baseline_di);
                    LOG_INFO("Pex1202L", std::string(buf));
                }

                // Si cambia respecto al último, log de diagnóstico
                if (di_state != last_di) {
                    uint32_t xor_bits = (baseline_di ^ di_state);

                    char buf[200];
                    std::snprintf(buf, sizeof(buf),
                                  "Brake DI change detected | baseline=0x%08X current=0x%08X xor=0x%08X (mask candidate=xor)",
                                  baseline_di, di_state, xor_bits);
                    LOG_INFO("Pex1202L", std::string(buf));

                    // Consejo: si ves que el “reposo” real es diferente, puedes fijar baseline a reposo.
                    // Por ahora mantenemos el baseline inicial.
                    last_di = di_state;
                }

                // 3) Por ahora: activo-alto FORZADO.
                //    Y usamos como máscara "candidata" el XOR con baseline.
                //    Si el XOR da 0 (no hay cambio), no activamos freno.
                //
                //    Nota: esto es para descubrir el bit correcto.
                //    Cuando sepamos el bit, se fija a (1u<<N) y listo.
                uint32_t mask_candidate = (baseline_di ^ di_state);

                bool brake_raw = false;
                if (mask_candidate != 0) {
                    // Activo-alto: si el bit que cambió está a 1 en el estado actual => freno = 1
                    brake_raw = (di_state & mask_candidate) != 0;
                } else {
                    // Si aún no hay cambio detectado, freno = 0
                    brake_raw = false;
                }

                // Activo-alto forzado (no invertimos)
                brake_switch_val = brake_raw ? 1.0 : 0.0;

                // 4) Log del valor interpretado (throttleado)
                static int brake_log = 0;
                if (brake_log++ % 20 == 0) {
                    char buf[160];
                    std::snprintf(buf, sizeof(buf),
                                  "Brake interpreted (active-high FORCED): %.1f | mask_candidate=0x%08X",
                                  brake_switch_val, mask_candidate);
                    LOG_INFO("Pex1202L", std::string(buf));
                }
            } else {
                static int err_log = 0;
                if (err_log++ % 50 == 0) {
                    LOG_WARN("Pex1202L", "Fallo leyendo DI (ioctl IXPIO_READ_DI)");
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
