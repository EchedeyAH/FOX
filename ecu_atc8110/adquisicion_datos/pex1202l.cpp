#include "pex1202l.hpp"
#include "../common/logging.hpp"
#include "pex1202_driver.hpp"
#include "pex_device.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>
#include <cstdio> // For fopen

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
        if (fd < 0) {
            LOG_WARN("Pex1202L", "Iniciando en MODO SIMULACIÓN por falta de hardware.");
            LOG_INFO("Pex1202L", "Control: echo <valor> > /tmp/force_accel");
            mock_mode_ = true;
            return true; // Éxito (simulado) para que el sistema arranque
        }
        return true;
    }

    void stop() override
    {
        // Singleton manages lifecycle
    }

    std::vector<common::AnalogSample> read_samples() override
    {
        std::vector<common::AnalogSample> samples;
        samples.reserve(config_.size() + 1); // +1 because we add brake_switch

        // --- 1. Digital Inputs (BRAKE_SW) ---
        // Access /dev/ixpio1 via PexDevice
        int dio_fd = PexDevice::GetInstance().GetDioFd();
        double brake_switch_val = 0.0;
        
        if (dio_fd >= 0) {
            uint32_t di_state = 0;
            // Define IOCTL constant locally if not in headers
            const unsigned long IXPIO_READ_DI = 0x40046901; 

            if (ioctl(dio_fd, IXPIO_READ_DI, &di_state) < 0) {
                // Log error occasionally?
            } else {
                // Debug log to help identify the correct bit for BRAKE_SW
                static int log_counter = 0;
                if (log_counter++ % 50 == 0) {
                    LOG_INFO("Pex1202L", "DI State: 0x" + common::to_hex_string(di_state));
                }

                // Assume Bit 0 for now, or just return the state 
                // We add it as a sensor named "brake_switch"
                // 1.0 if Bit 0 is set, else 0.0
                if (di_state & 0x01) {
                    brake_switch_val = 1.0;
                }
            }
        }
        
        // Add brake_switch to samples
        
        // [DEBUG] Override via file for remote testing
        FILE* f_brake = fopen("/tmp/force_brake", "r");
        if (f_brake) {
            int force_val = 0;
            if (fscanf(f_brake, "%d", &force_val) == 1 && force_val > 0) {
                brake_switch_val = 1.0;
            }
            fclose(f_brake);
        }

        samples.push_back({"brake_switch", brake_switch_val});


        if (mock_mode_) {
            // Modo Simulación: Leer archivo de override
            double accel_val = 0.0;
            FILE* f = fopen("/tmp/force_accel", "r");
            if (f) {
                float val = 0.0f;
                if (fscanf(f, "%f", &val) == 1) {
                    accel_val = static_cast<double>(val);
                }
                fclose(f);
            }

            for (const auto &channel : config_) {
                double val = 0.0;
                if (channel.name == "acelerador") val = accel_val;
                // Resto en 0
                samples.push_back({channel.name, val});
            }
            return samples;
        }

        // Modo Real (Analog Inputs)
        int fd = PexDevice::GetInstance().GetFd();
        if (fd < 0) return samples;

        bool any_data_read = false;

        for (const auto &channel : config_) {
            ixpci_reg reg;
            reg.id = IXPCI_AICR; 
            reg.value = channel.channel; 
            reg.mode = IXPCI_RM_NORMAL;
            
            if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
                 // Ignorar errores puntuales
            }

            reg.id = IXPCI_ADST;
            reg.value = 0; 
            reg.mode = IXPCI_RM_NORMAL;
            if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {}

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
            
            // [FIX] Normalize accelerator and brake to 0.0 - 1.0 range based on 12-bit ADC (0-4095)
            // unless scale is already small (e.g. < 0.1).
            // Assuming config scale 1.0 means "raw value". We need normalized for logic.
            if ((channel.name == "acelerador" || channel.name == "freno") && channel.scale > 0.5) {
                 value = value / 4095.0;
            }

            // [DEBUG] Override Accelerator via file (Real Mode)
            if (channel.name == "acelerador") {
                FILE* f_acc = fopen("/tmp/force_accel", "r");
                if (f_acc) {
                    float force_val = 0.0f;
                    if (fscanf(f_acc, "%f", &force_val) == 1) {
                         // Assume input in file is normalized 0.0-1.0
                         value = static_cast<double>(force_val);
                    }
                    fclose(f_acc);
                }
            }
            
            samples.push_back({channel.name, value});
            
            // [DEBUG] Log accelerator raw values
            if (channel.name == "acelerador") {
                static int acc_log_cnt = 0;
                if (acc_log_cnt++ % 50 == 0) {
                     LOG_INFO("Pex1202L", "ACC: Ch" + std::to_string(channel.channel) + 
                              " Raw=" + std::to_string(raw_val) + 
                              " Val=" + std::to_string(value));
                }
            }
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

std::unique_ptr<common::ISensorReader> CreatePex1202L(SensorConfig config)
{
    return std::make_unique<Pex1202L>(std::move(config));
}

} // namespace adquisicion_datos
