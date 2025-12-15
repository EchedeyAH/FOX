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

    // Prevent copying
    PexDevice(const PexDevice&) = delete;
    PexDevice& operator=(const PexDevice&) = delete;

    int GetFd() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fd_ >= 0) return fd_;

        LOG_INFO("PexDevice", "Opening PEX Device /dev/ixpci1...");
        fd_ = open("/dev/ixpci1", O_RDWR);
        
        if (fd_ < 0) {
            LOG_WARN("PexDevice", "Failed to open /dev/ixpci1, trying /dev/ixpci0...");
            fd_ = open("/dev/ixpci0", O_RDWR);
        }

        if (fd_ < 0) {
            LOG_WARN("PexDevice", "Hardware no encontrado. Se usará MODO SIMULACIÓN.");
            LOG_WARN("PexDevice", "Error original: " + std::string(strerror(errno)));
        } else {
             LOG_INFO("PexDevice", "Device opened successfully. FD: " + std::to_string(fd_));
        }

        return fd_;
    }

    int GetDioFd() {
        std::lock_guard<std::mutex> lock(dio_mutex_);
        if (dio_fd_ >= 0) return dio_fd_;

        LOG_INFO("PexDevice", "Opening PEX DIO Device /dev/ixpio1...");
        dio_fd_ = open("/dev/ixpio1", O_RDWR);

        if (dio_fd_ < 0) {
            LOG_WARN("PexDevice", "Failed to open /dev/ixpio1 (DIO). Error: " + std::string(strerror(errno)));
        } else {
            LOG_INFO("PexDevice", "DIO Device opened successfully. FD: " + std::to_string(dio_fd_));
        }
        
        return dio_fd_;
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
