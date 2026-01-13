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

    // Obtener FD para una tarjeta específica (index: 0, 1, etc.)
    int GetFd(int index = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (fds_.count(index) && fds_[index] >= 0) return fds_[index];

        std::string dev_path = "/dev/ixpci" + std::to_string(index);
        LOG_INFO("PexDevice", "Opening PEX Device " + dev_path + " (O_RDWR)...");
        
        int fd = open(dev_path.c_str(), O_RDWR);

        if (fd < 0) {
            int e = errno;
            LOG_WARN("PexDevice", "Fallo al abrir " + dev_path + ": errno=" + 
                                  std::to_string(e) + " (" + std::string(strerror(e)) + ")");
            // No hacemos fallback automático aquí para evitar confusiones entre tarjetas
            return -1;
        }

        LOG_INFO("PexDevice", "Tarjeta " + std::to_string(index) + " abierta. FD: " + std::to_string(fd));
        fds_[index] = fd;
        return fd;
    }

    // DIO (entradas): /dev/ixpioX
    int GetDioFd(int index = 1) {
        std::lock_guard<std::mutex> lock(dio_mutex_);
        if (dio_fds_.count(index) && dio_fds_[index] >= 0) return dio_fds_[index];

        std::string dev_path = "/dev/ixpio" + std::to_string(index);
        LOG_INFO("PexDevice", "Opening PEX DIO Device " + dev_path + " (O_RDWR)...");
        
        int fd = open(dev_path.c_str(), O_RDWR);

        if (fd < 0) {
            int e = errno;
            LOG_WARN("PexDevice", "Fallo al abrir " + dev_path + ": errno=" + 
                                  std::to_string(e) + " (" + std::string(strerror(e)) + ")");
            return -1;
        }

        LOG_INFO("PexDevice", "DIO Tarjeta " + std::to_string(index) + " abierta. FD: " + std::to_string(fd));
        dio_fds_[index] = fd;
        return fd;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : fds_) {
            if (pair.second >= 0) {
                close(pair.second);
                LOG_INFO("PexDevice", "Closed ixpci" + std::to_string(pair.first));
                pair.second = -1;
            }
        }
        std::lock_guard<std::mutex> lock2(dio_mutex_);
        for (auto& pair : dio_fds_) {
            if (pair.second >= 0) {
                close(pair.second);
                LOG_INFO("PexDevice", "Closed ixpio" + std::to_string(pair.first));
                pair.second = -1;
            }
        }
    }

private:
    PexDevice() = default;
    ~PexDevice() { Close(); }

    std::map<int, int> fds_;
    std::mutex mutex_;

    std::map<int, int> dio_fds_;
    std::mutex dio_mutex_;
};

} // namespace adquisicion_datos
