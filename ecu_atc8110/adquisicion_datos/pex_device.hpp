#pragma once

#include "../common/logging.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <map>
#include <set>
#include <string>
#include <cstring>
#include <cerrno>
#include <chrono>

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

        // Evitar re-intentar abrir si falló hace menos de 5 segundos
        auto now = std::chrono::steady_clock::now();
        if (failed_fds_.count(index)) {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_fail_pci_[index]).count() < 5) {
                return -1;
            }
        }

        std::string dev_path = "/dev/ixpci" + std::to_string(index);
        LOG_INFO("PexDevice", "Opening PEX Device " + dev_path + " (O_RDWR)...");
        
        int fd = open(dev_path.c_str(), O_RDWR);

        if (fd < 0) {
            int e = errno;
            failed_fds_.insert(index);
            last_fail_pci_[index] = now;
            LOG_WARN("PexDevice", "Fallo al abrir " + dev_path + ": errno=" + 
                                  std::to_string(e) + " (" + std::string(strerror(e)) + ")");
            return -1;
        }

        LOG_INFO("PexDevice", "Tarjeta " + std::to_string(index) + " abierta. FD: " + std::to_string(fd));
        fds_[index] = fd;
        failed_fds_.erase(index);
        return fd;
    }

    // DIO (entradas): /dev/ixpioX - Auto-detect working device
    int GetDioFd(int index = 1) {
        std::lock_guard<std::mutex> lock(dio_mutex_);
        
        // Si ya tenemos un FD válido, devolverlo
        if (dio_fds_.count(index) && dio_fds_[index] >= 0) return dio_fds_[index];

        // Si ya intentamos y falló permanentemente, no reintentar
        if (dio_permanently_failed_) {
            return -1;
        }

        LOG_INFO("PexDevice", "Auto-detecting working DIO device...");
        
        // Intentar abrir /dev/ixpio0 hasta /dev/ixpio15
        for (int i = 0; i <= 15; i++) {
            std::string dev_path = "/dev/ixpio" + std::to_string(i);
            
            // Intentar O_RDWR primero
            int fd = open(dev_path.c_str(), O_RDWR);
            
            if (fd < 0) {
                int e = errno;
                // ENXIO (6) = dispositivo no existe o no es compatible
                // ENOENT (2) = archivo no existe
                if (e == ENXIO || e == ENOENT) {
                    continue; // Probar siguiente
                }
                
                // EBUSY (16) = dispositivo ocupado, intentar solo lectura
                if (e == EBUSY) {
                    LOG_WARN("PexDevice", dev_path + " busy, trying O_RDONLY...");
                    fd = open(dev_path.c_str(), O_RDONLY);
                    if (fd < 0) {
                        continue;
                    }
                }
            }
            
            // Si llegamos aquí, el dispositivo se abrió correctamente
            LOG_INFO("PexDevice", "DIO Device found: " + dev_path + " (FD: " + std::to_string(fd) + ")");
            dio_fds_[index] = fd;
            dio_detected_index_ = i;
            return fd;
        }

        // No se encontró ningún dispositivo funcional
        LOG_ERROR("PexDevice", "No working DIO device found in /dev/ixpio0-15");
        dio_permanently_failed_ = true;
        return -1;
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
    std::set<int> failed_fds_;
    std::map<int, std::chrono::steady_clock::time_point> last_fail_pci_;
    std::mutex mutex_;

    std::map<int, int> dio_fds_;
    std::set<int> failed_dio_;
    std::map<int, std::chrono::steady_clock::time_point> last_fail_dio_;
    std::mutex dio_mutex_;
    bool dio_permanently_failed_ = false;
    int dio_detected_index_ = -1;
};

} // namespace adquisicion_datos
