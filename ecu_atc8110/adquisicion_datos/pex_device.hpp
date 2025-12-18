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

    // FD principal para /dev/ixpciX (AI/AO del driver ixpci)
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
            int e = errno;
            LOG_WARN("PexDevice", "Hardware no encontrado. Se usará MODO SIMULACIÓN.");
            LOG_WARN("PexDevice", "Error original: errno=" + std::to_string(e) +
                                  " (" + std::string(strerror(e)) + ")");
        } else {
            LOG_INFO("PexDevice", "Device opened successfully. FD: " + std::to_string(fd_));
        }

        return fd_;
    }

    // DIO (DI/DO): en tu proyecto este FD se usa tanto para leer DI (Pex1202L)
    // como para escribir DO (ENABLE en PexDa16). Por eso debe ser O_RDWR.
    int GetDioFd() {
        std::lock_guard<std::mutex> lock(dio_mutex_);
        if (dio_fd_ >= 0) return dio_fd_;

        const char* dev = "/dev/ixpio1";

        LOG_INFO("PexDevice", std::string("Opening PEX DIO Device ") + dev + " (O_RDWR)...");
        dio_fd_ = open(dev, O_RDWR);

        if (dio_fd_ < 0) {
            int e = errno;
            LOG_WARN("PexDevice", std::string("Failed opening ") + dev +
                                  " with O_RDWR. errno=" + std::to_string(e) +
                                  " (" + std::string(strerror(e)) + "). Trying O_RDONLY...");

            dio_fd_ = open(dev, O_RDONLY);
        }

        if (dio_fd_ < 0) {
            int e = errno;
            LOG_ERROR("PexDevice", std::string("Failed to open ") + dev +
                                  " (DIO). errno=" + std::to_string(e) +
                                  " (" + std::string(strerror(e)) + ")");
            return -1;
        }

        LOG_INFO("PexDevice", "DIO Device opened successfully. FD: " + std::to_string(dio_fd_));
        return dio_fd_;
    }

    // Si quieres separar lectura y escritura, usa este FD para escritura.
    // Lo alineamos a /dev/ixpio1 para que no haya incoherencias.
    int GetDioWriteFd() {
        std::lock_guard<std::mutex> lock(dio_wr_mutex_);
        if (dio_wr_fd_ >= 0) return dio_wr_fd_;

        const char* dev = "/dev/ixpio1";

        // Intento O_RDWR primero (algunos drivers requieren RW incluso para ciertos ioctls)
        LOG_INFO("PexDevice", std::string("Opening PEX DIO Device ") + dev + " (write, O_RDWR)...");
        dio_wr_fd_ = open(dev, O_RDWR);

        if (dio_wr_fd_ < 0) {
            int e = errno;
            LOG_WARN("PexDevice", std::string("Failed opening ") + dev +
                                  " with O_RDWR for write. errno=" + std::to_string(e) +
                                  " (" + std::string(strerror(e)) + "). Trying O_WRONLY...");

            dio_wr_fd_ = open(dev, O_WRONLY);
        }

        if (dio_wr_fd_ < 0) {
            int e = errno;
            LOG_ERROR("PexDevice", std::string("Failed to open ") + dev +
                                  " (DIO-WR). errno=" + std::to_string(e) +
                                  " (" + std::string(strerror(e)) + ")");
            return -1;
        }

        LOG_INFO("PexDevice", "DIO Device (write) opened successfully. FD: " + std::to_string(dio_wr_fd_));
        return dio_wr_fd_;
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
        {
            std::lock_guard<std::mutex> lock(dio_wr_mutex_);
            if (dio_wr_fd_ >= 0) {
                close(dio_wr_fd_);
                dio_wr_fd_ = -1;
                LOG_INFO("PexDevice", "DIO Device (write) closed.");
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

    int dio_wr_fd_ = -1;
    std::mutex dio_wr_mutex_;
};

} // namespace adquisicion_datos
