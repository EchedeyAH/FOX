#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <ctime>
#include <iomanip>
#include <cstdio>

#include <unistd.h>

#include "../../adquisicion_datos/pexda16.hpp"

// Optional CAN monitoring
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>

namespace {

constexpr double SAFE_V_MIN = 0.0;
constexpr double SAFE_V_MAX = 1.0;
constexpr double TEST_VOLT_LOW = 0.3;
constexpr double TEST_VOLT_HIGH = 0.6;
constexpr int DEFAULT_STEP_SEC = 4;

std::atomic<bool> g_running{true};
common::IActuatorWriter* g_actuator = nullptr;

void handle_signal(int) {
    g_running.store(false);
    if (g_actuator) {
        // best-effort safety reset
        for (int ch = 0; ch <= 7; ++ch) {
            g_actuator->write_output("AO" + std::to_string(ch), 0.0);
        }
    }
}

void set_all_zero(common::IActuatorWriter& act, int max_ch) {
    for (int ch = 0; ch <= max_ch; ++ch) {
        act.write_output("AO" + std::to_string(ch), 0.0);
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

std::string now_timestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    return std::string(buf);
}

std::string bytes_to_hex(const std::vector<uint8_t>& data) {
    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < data.size(); ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
        if (i + 1 < data.size()) ss << " ";
    }
    return ss.str();
}

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
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= duration_ms) break;
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

void save_mapping_json(const std::string& path, const std::map<int, std::string>& mapping) {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[WARN] No se pudo escribir " << path << "\n";
        return;
    }
    f << "{\n";
    f << "  \"timestamp\": \"" << now_timestamp() << "\",\n";
    f << "  \"mapping\": {\n";
    bool first = true;
    for (const auto& kv : mapping) {
        if (!first) f << ",\n";
        first = false;
        f << "    \"AO" << kv.first << "\": \"" << kv.second << "\"";
    }
    f << "\n  }\n";
    f << "}\n";
}

void save_mapping_csv(const std::string& path, const std::map<int, std::string>& mapping) {
    std::ofstream f(path);
    if (!f.is_open()) {
        std::cerr << "[WARN] No se pudo escribir " << path << "\n";
        return;
    }
    f << "channel,label\n";
    for (const auto& kv : mapping) {
        f << "AO" << kv.first << "," << kv.second << "\n";
    }
}

} // namespace

int main(int argc, char** argv) {
    bool manual_mode = false;
    bool dry_run = false;
    int manual_ch = -1;
    double manual_v = 0.0;
    int step_seconds = DEFAULT_STEP_SEC;
    std::string can_if;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--channel=", 0) == 0) {
            manual_mode = true;
            manual_ch = std::stoi(arg.substr(10));
        } else if (arg.rfind("--voltage=", 0) == 0) {
            manual_v = std::stod(arg.substr(10));
        } else if (arg.rfind("--step=", 0) == 0) {
            step_seconds = std::stoi(arg.substr(7));
        } else if (arg.rfind("--can=", 0) == 0) {
            can_if = arg.substr(6);
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Uso:\n"
                      << "  ./pex_da16_diag                (scan AO0..AO7)\n"
                      << "  ./pex_da16_diag --channel=3 --voltage=0.6\n"
                      << "Opciones:\n"
                      << "  --step=4           (segundos)\n"
                      << "  --can=can0         (monitor CAN opcional)\n"
                      << "  --dry-run          (no activa salidas)\n";
            return 0;
        }
    }

    if (manual_mode && (manual_ch < 0 || manual_ch > 7)) {
        std::cerr << "[ERR] Canal inválido. Use 0..7\n";
        return 1;
    }
    if (manual_mode && (manual_v < SAFE_V_MIN || manual_v > SAFE_V_MAX)) {
        std::cerr << "[ERR] Voltaje fuera de rango seguro (0.0V - 1.0V)\n";
        return 1;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    auto act = adquisicion_datos::CreatePexDa16();
    if (!act) {
        std::cerr << "[ERROR] No se pudo crear PEX-DA16\n";
        return 1;
    }
    if (!dry_run && !act->start()) {
        std::cerr << "[ERROR] No se pudo inicializar PEX-DA16\n";
        return 1;
    }
    g_actuator = act.get();
    SafetyGuard guard{act.get(), 7};
    std::atexit([](){
        if (g_actuator) {
            for (int ch = 0; ch <= 7; ++ch) {
                g_actuator->write_output("AO" + std::to_string(ch), 0.0);
            }
        }
    });

    int can_fd = -1;
    if (!can_if.empty()) {
        can_fd = open_can(can_if);
        if (can_fd < 0) {
            std::cerr << "[WARN] No se pudo abrir CAN en " << can_if << "\n";
        }
    }

    if (!dry_run) {
        set_all_zero(*act, 7);
    }

    if (manual_mode) {
        std::cout << "[INFO] Modo manual: AO" << manual_ch << " = " << manual_v << "V\n";
        std::cout << "[INFO] Ctrl+C para detener...\n";
        if (!dry_run) {
            act->write_output("AO" + std::to_string(manual_ch), manual_v);
        } else {
            std::cout << "[INFO] (dry-run) No se activan salidas\n";
        }
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if (!dry_run) {
            set_all_zero(*act, 7);
            act->stop();
        }
        if (can_fd >= 0) close(can_fd);
        return 0;
    }

    std::map<int, std::string> mapping;

    for (int ch = 0; ch <= 7 && g_running.load(); ++ch) {
        std::cout << "\n========================\n";
        std::cout << "Testing AO" << ch << "\n";
        std::cout << "Output = 0.6V\n";
        std::cout << "========================\n";
        std::cout << "[INFO] Observe vehicle behavior (motor, brake, CAN)\n";

        std::map<uint32_t, std::vector<uint8_t>> baseline;
        if (can_fd >= 0) {
            baseline = read_can_snapshot(can_fd, 1000);
        }

        if (!dry_run) {
            set_all_zero(*act, 7);
        }

        auto apply_step = [&](double v, const char* label) {
            if (!dry_run) {
                act->write_output("AO" + std::to_string(ch), v);
            }
            for (int t = step_seconds; t > 0 && g_running.load(); --t) {
                std::cout << "[INFO] " << label << "... " << t << "s\n";
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        };

        apply_step(0.0, "Applying signal");
        apply_step(TEST_VOLT_LOW, "Applying signal");
        apply_step(TEST_VOLT_HIGH, "Applying signal");
        apply_step(0.0, "Applying signal");

        if (can_fd >= 0) {
            auto during = read_can_snapshot(can_fd, 1000);
            auto changed = diff_can(baseline, during);
            if (!changed.empty()) {
                std::cout << "[CAN CHANGE]\n";
                for (auto id : changed) {
                    std::cout << "  ID 0x" << std::hex << id << std::dec << " changed\n";
                    auto it_b = baseline.find(id);
                    auto it_d = during.find(id);
                    if (it_b != baseline.end() && it_d != during.end()) {
                        std::cout << "    prev: " << bytes_to_hex(it_b->second) << "\n";
                        std::cout << "    new : " << bytes_to_hex(it_d->second) << "\n";
                    }
                }
                std::cout << "[ACTIVE] possible motor response\n";
            }
        }

        if (!dry_run) {
            set_all_zero(*act, 7);
        }
        std::this_thread::sleep_for(std::chrono::seconds(step_seconds));
    }

    std::cout << "\nIntroduce el mapeo manual:\n";
    for (int ch = 0; ch <= 7; ++ch) {
        std::cout << "AO" << ch << " → (enter function, e.g. motor_fl_accel): ";
        std::string label;
        std::getline(std::cin, label);
        if (label.empty()) label = "unknown";
        mapping[ch] = label;
    }

    save_mapping_json("ecu_output_map.json", mapping);
    save_mapping_csv("ecu_output_map.csv", mapping);
    std::cout << "[INFO] Guardado: ecu_output_map.json\n";
    std::cout << "[INFO] Guardado: ecu_output_map.csv\n";

    if (!dry_run) {
        set_all_zero(*act, 7);
        act->stop();
    }
    if (can_fd >= 0) close(can_fd);
    return 0;
}
