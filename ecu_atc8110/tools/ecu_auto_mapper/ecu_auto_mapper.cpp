#include <atomic>
#include <chrono>
#include <csignal>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <unistd.h>

#include "../../adquisicion_datos/pex1202l.hpp"
#include "../../adquisicion_datos/pexda16.hpp"

namespace {

constexpr double SAFE_V_MIN = 0.0;
constexpr double SAFE_V_MAX = 0.4;
constexpr double TEST_VOLT = 0.4;
constexpr int BASELINE_MS = 1000;
constexpr int STEP_MS = 1000;
constexpr int SETTLE_MS = 200;
constexpr int READ_INTERVAL_MS = 50;
constexpr double DETECT_THRESHOLD = 0.1;

std::atomic<bool> g_running{true};
common::IActuatorWriter* g_actuator = nullptr;

void set_all_zero(common::IActuatorWriter& act, int max_ch) {
    for (int ch = 0; ch <= max_ch; ++ch) {
        act.write_output("AO" + std::to_string(ch), 0.0);
    }
}

void handle_signal(int) {
    g_running.store(false);
    if (g_actuator) {
        set_all_zero(*g_actuator, 7);
    }
}

struct SafetyGuard {
    common::IActuatorWriter* act{nullptr};
    int max_ch{7};
    ~SafetyGuard() {
        if (act) {
            set_all_zero(*act, max_ch);
        }
    }
};

std::unordered_map<std::string, double> compute_baseline(common::ISensorReader& adc) {
    std::unordered_map<std::string, double> sum;
    std::unordered_map<std::string, int> count;
    auto start = std::chrono::steady_clock::now();
    while (g_running.load()) {
        auto samples = adc.read_samples();
        for (const auto& s : samples) {
            sum[s.name] += s.value;
            count[s.name] += 1;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= BASELINE_MS) break;
        usleep(READ_INTERVAL_MS * 1000);
    }
    std::unordered_map<std::string, double> baseline;
    for (const auto& kv : sum) {
        int c = count[kv.first];
        if (c > 0) {
            baseline[kv.first] = kv.second / static_cast<double>(c);
        }
    }
    return baseline;
}

} // namespace

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

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
        std::cerr << "[ERROR] ADC init failed\n";
        return 1;
    }

    auto act = adquisicion_datos::CreatePexDa16();
    if (!act || !act->start()) {
        std::cerr << "[ERROR] DA16 init failed\n";
        return 1;
    }

    g_actuator = act.get();
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    SafetyGuard guard{act.get(), 7};

    set_all_zero(*act, 7);
    usleep(SETTLE_MS * 1000);

    std::cout << "[INFO] Baseline ADC (" << BASELINE_MS << " ms)...\n";
    auto baseline = compute_baseline(*adc);
    if (baseline.empty()) {
        std::cerr << "[ERROR] No baseline samples\n";
        return 1;
    }

    std::map<int, std::string> mapping;

    for (int ch = 0; ch <= 7 && g_running.load(); ++ch) {
        std::cout << "\nTesting AO" << ch << "\n";
        set_all_zero(*act, 7);
        usleep(SETTLE_MS * 1000);

        // Apply 0.4V safely to one channel only
        double v = TEST_VOLT;
        if (v < SAFE_V_MIN) v = SAFE_V_MIN;
        if (v > SAFE_V_MAX) v = SAFE_V_MAX;
        act->write_output("AO" + std::to_string(ch), v);

        std::unordered_map<std::string, double> max_delta;
        auto start = std::chrono::steady_clock::now();
        while (g_running.load()) {
            auto samples = adc->read_samples();
            for (const auto& s : samples) {
                auto it = baseline.find(s.name);
                if (it != baseline.end()) {
                    double d = std::abs(s.value - it->second);
                    if (d > max_delta[s.name]) {
                        max_delta[s.name] = d;
                    }
                }
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= STEP_MS) break;
            usleep(READ_INTERVAL_MS * 1000);
        }

        act->write_output("AO" + std::to_string(ch), 0.0);
        usleep(SETTLE_MS * 1000);

        std::vector<std::pair<std::string, double>> detected;
        for (const auto& kv : max_delta) {
            if (kv.second > DETECT_THRESHOLD) {
                detected.emplace_back(kv.first, kv.second);
            }
        }

        if (detected.empty()) {
            std::cout << "Detected change in: none\n";
            mapping[ch] = "unknown";
        } else {
            std::cout << "Detected change in:\n";
            std::ostringstream label;
            for (size_t i = 0; i < detected.size(); ++i) {
                const auto& d = detected[i];
                std::cout << "  " << d.first << " Δ=" << std::fixed << std::setprecision(3)
                          << d.second << "\n";
                if (i > 0) label << ",";
                label << d.first;
            }
            mapping[ch] = label.str();
        }
    }

    std::cout << "\nDetected relationships:\n";
    for (const auto& kv : mapping) {
        std::cout << "AO" << kv.first << " -> " << kv.second << "\n";
    }

    // Save JSON result to stdout-friendly file
    std::ofstream f("ecu_output_map.json");
    if (f.is_open()) {
        f << "{\n";
        bool first = true;
        for (const auto& kv : mapping) {
            if (!first) f << ",\n";
            first = false;
            f << "  \"AO" << kv.first << "\": \"" << kv.second << "\"";
        }
        f << "\n}\n";
        std::cout << "[INFO] Mapping saved to ecu_output_map.json\n";
    } else {
        std::cerr << "[WARN] Could not write ecu_output_map.json\n";
    }

    return 0;
}
