#include <iostream>
#include <iomanip>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <cmath>
#include <chrono>
#include <ctime>
#include <sstream>

#include "adquisicion_datos/pex1202l.hpp"

int main(int argc, char** argv)
{
    bool debug = false;
    if (argc > 1 && std::string(argv[1]) == "--debug") {
        debug = true;
    }

    std::cout << "Test Simple PEX-1202L (vía driver de ECU)" << std::endl;
    std::cout << "========================================" << std::endl;

    adquisicion_datos::SensorConfig config = {
        {"acelerador", 1, 1.0, 0.0},
        {"corriente_eje_d", 2, 1.0, 0.0},
        {"freno", 3, 1.0, 0.0},
        {"volante", 4, 1.0, 0.0},
        {"corriente_eje_t", 5, 1.0, 0.0},
        {"suspension_rl", 6, 1.0, 0.0},
        {"suspension_rr", 8, 1.0, 0.0},
        {"suspension_fl", 10, 1.0, 0.0},
        {"suspension_fr", 12, 1.0, 0.0}
    };

    auto adc = adquisicion_datos::CreatePex1202L(config);
    if (!adc || !adc->start()) {
        std::cerr << "ADC init failed" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, double> prev;
    std::unordered_map<std::string, int> zero_count;
    std::unordered_map<std::string, int> noise_count;
    std::unordered_map<std::string, int> warn_rate_limit_zero;
    std::unordered_map<std::string, int> warn_rate_limit_noise;

    const double zero_threshold = 0.01;
    const double change_threshold = 0.1;
    const double noise_low = 0.01;
    const double noise_high = 0.05;
    const int noise_cycles_required = 10;

    while (true) {
        auto samples = adc->read_samples();

        // Clear screen (dashboard)
        std::cout << "\033[H\033[J";

        // Timestamp line
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &tt);
#else
        localtime_r(&tt, &tm);
#endif
        std::ostringstream ts;
        ts << std::put_time(&tm, "[%H:%M:%S]");
        std::cout << ts.str() << "\n";

        std::cout << std::left
                  << std::setw(20) << "CANAL"
                  << std::setw(12) << "VOLTAJE"
                  << std::setw(12) << "ESTADO"
                  << std::endl;
        std::cout << std::string(44, '-') << std::endl;

        if (samples.empty()) {
            std::cerr << "[ERROR] No samples received" << std::endl;
            usleep(100000);
            continue;
        }

        for (const auto& s : samples) {
            const auto it = prev.find(s.name);
            double delta = 0.0;
            if (it != prev.end()) {
                delta = std::abs(s.value - it->second);
            }

            // Stuck-at-zero detection
            if (std::abs(s.value) < zero_threshold) {
                zero_count[s.name]++;
            } else {
                zero_count[s.name] = 0;
            }

            // Noise detection (small continuous changes)
            if (delta > noise_low && delta <= noise_high) {
                noise_count[s.name]++;
            } else {
                noise_count[s.name] = 0;
            }

            std::string status = "OK";
            if (zero_count[s.name] >= 5) {
                status = "WARN";
                if (warn_rate_limit_zero[s.name] % 10 == 0) {
                    std::cerr << "[WARN] " << s.name << " stuck at 0" << std::endl;
                }
                warn_rate_limit_zero[s.name]++;
            } else if (s.value > 0.2) {
                status = "ACTIVE";
            } else if (delta > change_threshold) {
                status = "CHANGE";
            } else if (noise_count[s.name] >= noise_cycles_required) {
                status = "NOISE";
                if (warn_rate_limit_noise[s.name] % 10 == 0) {
                    std::cerr << "[WARN] " << s.name << " unstable signal" << std::endl;
                }
                warn_rate_limit_noise[s.name]++;
            }

            std::cout << std::left
                      << std::setw(20) << s.name
                      << std::fixed << std::setprecision(3)
                      << std::setw(12) << s.value << " V"
                      << std::setw(12) << status
                      << std::endl;

            if (debug) {
                std::cout << "  (debug) name=" << s.name
                          << " value=" << std::fixed << std::setprecision(3)
                          << s.value << " delta=" << std::fixed << std::setprecision(3)
                          << delta << std::endl;
            }

            prev[s.name] = s.value;
        }

        std::cout << std::endl;
        usleep(100000); // 100 ms
    }

    return 0;
}
