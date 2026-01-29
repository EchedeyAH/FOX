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
        reg.value = PEX_GAIN_BIP_5V;
        reg.mode = IXPCI_RM_NORMAL;

        if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
            LOG_WARN("Pex1202L", "No se pudo configurar rango ±5V");
        }

        return true;
    }

    void stop() override {}

    std::vector<common::AnalogSample> read_samples() override {
        std::vector<common::AnalogSample> samples;
        samples.reserve(config_.size() + 1);
        // ===== DEBUG: ESCANEO DE TODOS LOS CANALES =====
        static int scan_cnt = 0;
        if (scan_cnt++ % 20 == 0) {   // cada ~1s
            int fd = PexDevice::GetInstance().GetFd(1);
            if (fd >= 0) {
                for (int ch = 0; ch < 16; ++ch) {
                    ixpci_reg reg{};

                    // seleccionar canal
                    reg.id = IXPCI_AICR;
                    reg.value = ch;
                    reg.mode = IXPCI_RM_NORMAL;
                    ioctl(fd, IXPCI_WRITE_REG, &reg);

                    // start conversion
                    reg.id = IXPCI_ADST;
                    reg.value = 0;
                    ioctl(fd, IXPCI_WRITE_REG, &reg);

                    // read
                    reg.id = IXPCI_AI;
                    reg.mode = IXPCI_RM_READY;
                    ioctl(fd, IXPCI_READ_REG, &reg);

                    uint16_t raw = reg.value & 0x0FFF;
                    double volts = (raw - 2048.0) * (5.0 / 2048.0);

                    LOG_INFO("ADC_SCAN",
                        "CH" + std::to_string(ch) +
                        " RAW=" + std::to_string(raw) +
                        " V=" + std::to_string(volts));
                }
            }
        }

        /* ===== MODO SIMULACIÓN ===== */
        if (mock_mode_) {
            for (const auto& channel : config_) {
                samples.push_back({channel.name, 0.0});
            }
            return samples;
        }

        /* ===== LECTURA ANALÓGICA ===== */
        int fd = PexDevice::GetInstance().GetFd(1);
        if (fd < 0) return samples;

        for (const auto& channel : config_) {
            ixpci_reg reg{};

            reg.id = IXPCI_AICR;
            reg.value = channel.channel;
            reg.mode = IXPCI_RM_NORMAL;
            ioctl(fd, IXPCI_WRITE_REG, &reg);

            reg.id = IXPCI_ADST;
            reg.value = 0;
            ioctl(fd, IXPCI_WRITE_REG, &reg);

            reg.id = IXPCI_AI;
            reg.mode = IXPCI_RM_READY;
            ioctl(fd, IXPCI_READ_REG, &reg);

            uint16_t raw = reg.value & 0x0FFF;
            double volts = (raw - 2048.0) * (5.0 / 2048.0);
            if (volts < 0.0) volts = 0.0;

            double value = volts;
            if (channel.name == "acelerador" || channel.name == "freno") {
                value = volts / 5.0;
                if (value > 1.0) value = 1.0;
            } else {
                value = volts * channel.scale + channel.offset;
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
