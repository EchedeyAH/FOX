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
            LOG_ERROR("PexDevice", "CRITICAL: Failed to open DAQ device: " + std::string(strerror(errno)));
        } else {
             LOG_INFO("PexDevice", "Device opened successfully. FD: " + std::to_string(fd_));
        }

        return fd_;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
            LOG_INFO("PexDevice", "Device closed.");
        }
    }

private:
    PexDevice() = default;
    ~PexDevice() { Close(); }

    int fd_ = -1;
    std::mutex mutex_;
};

} // namespace adquisicion_datos
