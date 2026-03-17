#include <atomic>
#include <chrono>
#include <csignal>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <unistd.h>

#include "../../adquisicion_datos/pex1202l.hpp"
#include "../../adquisicion_datos/pexda16.hpp"

// Optional CAN (SocketCAN)
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

namespace {

constexpr double SAFE_V_MIN = 0.0;
constexpr double SAFE_V_MAX = 0.6;
constexpr double STEP_V1 = 0.3;
constexpr double STEP_V2 = 0.6;
constexpr int BASELINE_MS = 1000;
constexpr int STEP_MS = 1000;
constexpr int SETTLE_MS = 200;
constexpr int READ_INTERVAL_MS = 50;
constexpr double DETECT_THRESHOLD = 0.2;

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

int open_can(const std::string& ifname) {
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) return -1;
    struct ifreq ifr{};
    std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname.c_str());
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        close(s);
        return -1;
    }
    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(s);
        return -1;
    }
    // Non-blocking
    int flags = fcntl(s, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(s, F_SETFL, flags | O_NONBLOCK);
    }
    return s;
}

std::map<uint32_t, std::vector<uint8_t>> read_can_snapshot(int s, int duration_ms) {
    std::map<uint32_t, std::vector<uint8_t>> snap;
    auto start = std::chrono::steady_clock::now();
    while (g_running.load()) {
        struct can_frame frame{};
        int n = read(s, &frame, sizeof(frame));
        if (n > 0) {
            uint32_t id = frame.can_id & CAN_EFF_MASK;
            std::vector<uint8_t> data(frame.data, frame.data + frame.can_dlc);
            snap[id] = data;
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= duration_ms) break;
        usleep(1000);
    }
    return snap;
}

std::vector<uint32_t> diff_can(const std::map<uint32_t, std::vector<uint8_t>>& a,
                               const std::map<uint32_t, std::vector<uint8_t>>& b) {
    std::vector<uint32_t> changed;
    for (const auto& kv : b) {
        auto it = a.find(kv.first);
        if (it == a.end() || it->second != kv.second) {
            changed.push_back(kv.first);
        }
    }
    return changed;
}

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

std::string classify_type(const std::vector<std::string>& adc_names,
                          const std::vector<uint32_t>& can_ids) {
    bool has_current = false;
    for (const auto& n : adc_names) {
        if (n.find("corriente") != std::string::npos) {
            has_current = true;
            break;
        }
    }
    if (has_current) return "motor";
    if (adc_names.empty() && !can_ids.empty()) return "digital_control";
    if (!adc_names.empty()) return "analog_control";
    return "unknown";
}

std::string join_names(const std::vector<std::string>& names) {
    std::ostringstream ss;
    for (size_t i = 0; i < names.size(); ++i) {
        if (i > 0) ss << ",";
        ss << names[i];
    }
    return ss.str();
}

std::string join_can(const std::vector<uint32_t>& ids) {
    std::ostringstream ss;
    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) ss << ",";
        ss << "0x" << std::hex << std::uppercase << ids[i] << std::dec;
    }
    return ss.str();
}

struct MapEntry {
    std::vector<std::string> adc;
    std::vector<uint32_t> can_ids;
    std::string type;
};

} // namespace

int main(int argc, char** argv) {
    std::string can_if;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--can=", 0) == 0) {
            can_if = arg.substr(6);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Uso: ./ecu_auto_mapper [--can=can0]\n";
            return 0;
        }
    }

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

    int can_socket = -1;
    if (!can_if.empty()) {
        can_socket = open_can(can_if);
        if (can_socket < 0) {
            std::cerr << "[WARN] CAN no disponible en " << can_if << "\n";
        }
    }

    set_all_zero(*act, 7);
    usleep(SETTLE_MS * 1000);

    std::cout << "[INFO] Baseline ADC (" << BASELINE_MS << " ms)...\n";
    auto baseline = compute_baseline(*adc);
    if (baseline.empty()) {
        std::cerr << "[ERROR] No baseline samples\n";
        return 1;
    }

    std::map<int, MapEntry> mapping;

    for (int ch = 0; ch <= 7 && g_running.load(); ++ch) {
        std::cout << "[INFO] Testing AO" << ch << "\n";
        set_all_zero(*act, 7);
        usleep(SETTLE_MS * 1000);

        std::map<uint32_t, std::vector<uint8_t>> can_before;
        if (can_socket >= 0) {
            can_before = read_can_snapshot(can_socket, 300);
        }

        // 0 -> 0.3 -> 0.6 -> 0
        const double steps[] = {0.0, STEP_V1, STEP_V2, 0.0};
        std::unordered_map<std::string, double> max_delta;

        for (double v_in : steps) {
            double v = v_in;
            if (v < SAFE_V_MIN) v = SAFE_V_MIN;
            if (v > SAFE_V_MAX) v = SAFE_V_MAX;
            act->write_output("AO" + std::to_string(ch), v);

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
        }

        act->write_output("AO" + std::to_string(ch), 0.0);
        usleep(SETTLE_MS * 1000);

        std::vector<std::string> detected_adc;
        for (const auto& kv : max_delta) {
            if (kv.second > DETECT_THRESHOLD) {
                detected_adc.push_back(kv.first);
                std::cout << "[INFO] ADC change detected: " << kv.first
                          << " +" << std::fixed << std::setprecision(2)
                          << kv.second << "V\n";
            }
        }

        std::vector<uint32_t> can_changed;
        if (can_socket >= 0) {
            auto can_after = read_can_snapshot(can_socket, 300);
            can_changed = diff_can(can_before, can_after);
            for (auto id : can_changed) {
                std::cout << "[INFO] CAN change detected: ID 0x"
                          << std::hex << std::uppercase << id << std::dec << "\n";
            }
        }

        MapEntry entry;
        entry.adc = detected_adc;
        entry.can_ids = can_changed;
        entry.type = classify_type(detected_adc, can_changed);
        mapping[ch] = entry;
    }

    std::cout << "\nDetected relationships:\n";
    for (const auto& kv : mapping) {
        std::cout << "AO" << kv.first << " -> ";
        if (!kv.second.adc.empty()) {
            std::cout << join_names(kv.second.adc);
        } else {
            std::cout << "(no_adc_change)";
        }
        if (!kv.second.can_ids.empty()) {
            std::cout << " | CAN: " << join_can(kv.second.can_ids);
        }
        std::cout << " | type=" << kv.second.type << "\n";
    }

    // JSON output
    std::ofstream jf("ecu_auto_map.json");
    if (jf.is_open()) {
        jf << "{\n";
        bool first = true;
        for (const auto& kv : mapping) {
            if (!first) jf << ",\n";
            first = false;
            jf << "  \"AO" << kv.first << "\": {\n";
            jf << "    \"adc\": [";
            for (size_t i = 0; i < kv.second.adc.size(); ++i) {
                if (i > 0) jf << ", ";
                jf << "\"" << kv.second.adc[i] << "\"";
            }
            jf << "],\n";
            jf << "    \"can_ids\": [";
            for (size_t i = 0; i < kv.second.can_ids.size(); ++i) {
                if (i > 0) jf << ", ";
                jf << "\"0x" << std::hex << std::uppercase << kv.second.can_ids[i] << std::dec << "\"";
            }
            jf << "],\n";
            jf << "    \"type\": \"" << kv.second.type << "\"\n";
            jf << "  }";
        }
        jf << "\n}\n";
        std::cout << "[INFO] JSON saved to ecu_auto_map.json\n";
    } else {
        std::cerr << "[WARN] Could not write ecu_auto_map.json\n";
    }

    // CSV output
    std::ofstream cf("ecu_auto_map.csv");
    if (cf.is_open()) {
        cf << "channel,type,adc,can_ids\n";
        for (const auto& kv : mapping) {
            cf << "AO" << kv.first << "," << kv.second.type << ","
               << join_names(kv.second.adc) << "," << join_can(kv.second.can_ids) << "\n";
        }
        std::cout << "[INFO] CSV saved to ecu_auto_map.csv\n";
    } else {
        std::cerr << "[WARN] Could not write ecu_auto_map.csv\n";
    }

    if (can_socket >= 0) close(can_socket);
    return 0;
}
