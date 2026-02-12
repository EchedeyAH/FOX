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
        int fd = PexDevice::GetInstance().GetFd(1);
        if (fd < 0) {
            LOG_WARN("Pex1202L", "MODO SIMULACIÓN: no se pudo abrir /dev/ixpci1");
            mock_mode_ = true;
            return true;
        }

        ixpci_reg reg{};
        reg.id = IXPCI_ADGCR;
        reg.value = PEX_GAIN_UNI_10V;  // Modo unipolar 0-10V
        reg.mode = IXPCI_RM_NORMAL;

        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
            LOG_WARN("Pex1202L", "No se pudo configurar rango unipolar 0-10V");
        }

        return true;
    }

    void stop() override {}

    std::vector<common::AnalogSample> read_samples() override {
        std::vector<common::AnalogSample> samples;
        samples.reserve(config_.size());

        /* ===== MODO SIMULACIÓN ===== */
        if (mock_mode_) {
            for (const auto& channel : config_) {
                samples.push_back({channel.name, 0.0});
            }
            return samples;
        }

        /* ===== LECTURA ANALÓGICA CON HANDSHAKE MAGICSCAN ===== */
        int fd = PexDevice::GetInstance().GetFd(1);
        if (fd < 0) {
            LOG_ERROR("Pex1202L", "FD inválido");
            return samples;
        }

        // DEBUG: Logging detallado
        static int debug_count = 0;
        bool debug_this_cycle = (debug_count++ % 20 == 0);

        // IMPORTANTE: Lectura dummy en AI_15 para estabilizar MUX
        if (debug_this_cycle) LOG_INFO("Pex1202L", "Seleccionando canal dummy AI_15...");
        if (pex1202::select_channel(fd, AD_CHANNEL_DUMMY, AD_CONFIG_UNIPOLAR_5V) < 0) {
            LOG_ERROR("Pex1202L", "FALLO: Handshake en canal dummy");
            return samples;
        }
        
        int dummy_val = pex1202::read_adc(fd);
        if (dummy_val < 0) {
            LOG_ERROR("Pex1202L", "FALLO: Lectura ADC dummy");
            return samples;
        }
        if (debug_this_cycle) LOG_INFO("Pex1202L", "Dummy OK: raw=" + std::to_string(dummy_val));

        // Leer los canales configurados
        for (const auto& channel : config_) {
            // Determinar configuración bipolar/unipolar según canal
            int config_code = AD_CONFIG_UNIPOLAR_5V;  // default
            float v_max = 5.0f;
            
            if (channel.name == "corriente_eje_d" || channel.name == "corriente_eje_t") {
                config_code = AD_CONFIG_BIPOLAR_5V;
                v_max = 5.0f;
            }

            // Seleccionar canal con handshake
            if (pex1202::select_channel(fd, channel.channel, config_code) < 0) {
                LOG_ERROR("Pex1202L", "FALLO handshake CH" + std::to_string(channel.channel) + " (" + channel.name + ")");
                samples.push_back({channel.name, 0.0});
                continue;
            }

            // Leer ADC
            int raw = pex1202::read_adc(fd);
            if (raw < 0) {
                LOG_ERROR("Pex1202L", "FALLO ADC CH" + std::to_string(channel.channel) + " (" + channel.name + ")");
                samples.push_back({channel.name, 0.0});
                continue;
            }

            // Convertir a voltaje
            float voltage = pex1202::compute_voltage(raw, config_code, v_max);

            if (debug_this_cycle) {
                LOG_INFO("Pex1202L", 
                    "CH" + std::to_string(channel.channel) + " (" + channel.name + "): " +
                    "raw=" + std::to_string(raw) + " → " + std::to_string(voltage) + "V");
            }

            // Aplicar escalado y normalización
            double value = voltage;
            if (channel.name == "acelerador" || channel.name == "freno") {
                // Normalizar 0-5V a 0-1
                value = voltage / 5.0;
                if (value < 0.0) value = 0.0;
                if (value > 1.0) value = 1.0;
            } else if (channel.name == "corriente_eje_d" || channel.name == "corriente_eje_t") {
                // Currents are bipolar ±5V, scale/offset defined by SensorManager
                value = voltage * channel.scale + channel.offset;
            } else {
                // Other channels (suspensions, steering)
                value = voltage * channel.scale + channel.offset;
            }

            samples.push_back({channel.name, value});
        }

        return samples;
    }

private:
    SensorConfig config_;
    bool mock_mode_{false};
};

std::unique_ptr<common::ISensorReader>
CreatePex1202L(SensorConfig config) {
    return std::make_unique<Pex1202L>(std::move(config));
}

} // namespace adquisicion_datos
