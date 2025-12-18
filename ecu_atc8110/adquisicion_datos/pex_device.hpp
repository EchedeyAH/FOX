#pragma once

#include "../common/logging.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <string>
#include <cstring>
#include <cerrno>

namespace adquisicion_datos {

class PexDevice {
public:
    static PexDevice& GetInstance() {
        static PexDevice instance;
        return instance;
    }

    PexDevice(const PexDevice&) = delete;
    PexDevice& operator=(const PexDevice&) = delete;

    // FD principal: registros IXPCI_* (AI/AO/DO, etc.)
    int GetFd() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fd_ >= 0) return fd_;

        const char* devs[] = {"/dev/ixpci1", "/dev/ixpci0"};
        for (const char* dev : devs) {
            LOG_INFO("PexDevice", std::string("Opening PEX Device ") + dev + " (O_RDWR)...");
            fd_ = open(dev, O_RDWR | O_CLOEXEC);
            if (fd_ >= 0) {
                LOG_INFO("PexDevice", "Device opened successfully. FD: " + std::to_string(fd_));
                return fd_;
            }
            int e = errno;
            LOG_WARN("PexDevice", std::string("Failed to open ") + dev +
                                 ". errno=" + std::to_string(e) +
                                 " (" + std::string(strerror(e)) + ")");
        }

        // Si no hay hardware, se entra en simulación
        LOG_WARN("PexDevice", "Hardware no encontrado. Se usará MODO SIMULACIÓN.");
        return -1;
    }

    // FD DIO: ioctls IXPIO_* (DI, etc.) — abrir O_RDWR para compatibilidad de driver
    int GetDioFd() {
        std::lock_guard<std::mutex> lock(dio_mutex_);
        if (dio_fd_ >= 0) return dio_fd_;

        const char* devs[] = {"/dev/ixpio1", "/dev/ixpio0"};
        for (const char* dev : devs) {
            LOG_INFO("PexDevice", std::string("Opening PEX DIO Device ") + dev + " (O_RDWR)...");
            dio_fd_ = open(dev, O_RDWR | O_CLOEXEC);
            if (dio_fd_ >= 0) {
                LOG_INFO("PexDevice", "DIO Device opened successfully. FD: " + std::to_string(dio_fd_));
                return dio_fd_;
            }
            int e = errno;
            LOG_WARN("PexDevice", std::string("Failed to open ") + dev +
                                 " (DIO). errno=" + std::to_string(e) +
                                 " (" + std::string(strerror(e)) + ")");
        }

        LOG_ERROR("PexDevice", "No se pudo abrir ningún /dev/ixpioX (DIO).");
        return -1;
    }

    void Close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (fd_ >= 0) {
                close(fd_);
                fd_ = -1;
                LOG_INFO("PexDevice", "Device closed.");
            }
        }
        {
            std::lock_guard<std::mutex> lock(dio_mutex_);
            if (dio_fd_ >= 0) {
                close(dio_fd_);
                dio_fd_ = -1;
                LOG_INFO("PexDevice", "DIO Device closed.");
            }
        }
    }

private:
    PexDevice() = default;
    ~PexDevice() { Close(); }

    int fd_ = -1;
    std::mutex mutex_;

    int dio_fd_ = -1;
    std::mutex dio_mutex_;
};

} // namespace adquisicion_datos
