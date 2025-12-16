#pragma once

#include "../common/logging.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <string>
#include <cstring>
#include <cerrno>

// Nuevos includes para robustez DIO
#include <fstream>
#include <optional>
#include <vector>
#include <algorithm>
#include <glob.h>
#include <cctype>

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

        // 1) Autoridad del driver: /proc/ixpio/ixpio
        std::vector<std::string> candidates;

        if (auto procDev = ReadIxpioDevFromProc_()) {
            candidates.push_back(*procDev);
            LOG_INFO("PexDevice", "DIO candidate from /proc: " + *procDev);
        } else {
            LOG_WARN("PexDevice", "Could not read /proc/ixpio/ixpio (will scan /dev/ixpio*)");
        }

        // 2) Fallback: escaneo de nodos /dev/ixpio*
        auto scanned = GlobIxpioNodes_();
        for (auto &p : scanned) {
            if (std::find(candidates.begin(), candidates.end(), p) == candidates.end())
                candidates.push_back(p);
        }

        // 3) Último fallback: por si glob no devuelve nada
        for (const char* hard : {"/dev/ixpio0", "/dev/ixpio1"}) {
            std::string p(hard);
            if (std::find(candidates.begin(), candidates.end(), p) == candidates.end())
                candidates.push_back(p);
        }

        // 4) Probar candidatos en orden
        for (const auto& path : candidates) {
            LOG_INFO("PexDevice", "Opening PEX DIO Device " + path + " (Strict O_RDWR)...");
            int fd = open(path.c_str(), O_RDWR);

            if (fd >= 0) {
                dio_fd_ = fd;
                LOG_INFO("PexDevice",
                         "DIO Device opened successfully: " + path +
                         " | FD: " + std::to_string(dio_fd_));
                return dio_fd_;
            }

            const int e = errno;
            LOG_WARN("PexDevice",
                     "Failed to open " + path +
                     " (DIO). errno=" + std::to_string(e) +
                     " (" + std::string(strerror(e)) + ")");

            // ENXIO: nodo existe pero el driver rechaza ese minor/dispositivo
            if (e == ENXIO || e == ENODEV) {
                LOG_WARN("PexDevice", "Likely minor mismatch / wrong ixpio node. Trying next /dev/ixpio*.");
            }
        }

        LOG_ERROR("PexDevice", "No usable /dev/ixpio* device could be opened. BRAKE_SW will stay unavailable.");
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

    // Lee el nodo real desde /proc/ixpio/ixpio.
    // Heurística simple: busca token "ixpioN" y devuelve "/dev/ixpioN".
    static std::optional<std::string> ReadIxpioDevFromProc_() {
        std::ifstream f("/proc/ixpio/ixpio");
        if (!f.is_open()) return std::nullopt;

        std::string line;
        while (std::getline(f, line)) {
            auto pos = line.find("ixpio");
            if (pos == std::string::npos) continue;

            size_t end = pos;
            while (end < line.size() &&
                   (std::isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) {
                ++end;
            }

            std::string dev = line.substr(pos, end - pos); // "ixpio0"
            return "/dev/" + dev;
        }

        return std::nullopt;
    }

    static std::vector<std::string> GlobIxpioNodes_() {
        glob_t g{};
        std::vector<std::string> out;

        if (glob("/dev/ixpio*", 0, nullptr, &g) == 0) {
            for (size_t i = 0; i < g.gl_pathc; ++i) {
                out.emplace_back(g.gl_pathv[i]);
            }
        }
        globfree(&g);

        std::sort(out.begin(), out.end()); // orden estable
        return out;
    }

    int fd_ = -1;
    std::mutex mutex_;

    int dio_fd_ = -1;
    std::mutex dio_mutex_;
};

} // namespace adquisicion_datos
