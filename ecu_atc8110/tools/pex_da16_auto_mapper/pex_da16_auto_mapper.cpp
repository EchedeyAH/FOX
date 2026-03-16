#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>

extern "C" {
    #include "../adquisicion_datos/ixpci.h"
}

namespace {

constexpr int AO_RANGE_REG = 291;   // IXPCI_AO_CONFIGURATION
constexpr int AO_ENABLE_REG = 225;  // IXPCI_ENABLE_DISABLE_DA_CHANNEL
constexpr int AO_CH0_REG = 222;     // IXPCI_AO0
constexpr int AO_CH1_REG = 223;     // IXPCI_AO1
constexpr int AO_CH2_REG = 224;     // IXPCI_AO2
constexpr int AO_FALLBACK_REG = 220;

constexpr double SAFE_VOLT_MIN = 0.0;
constexpr double SAFE_VOLT_MAX = 1.0;
constexpr double DAC_FULL_SCALE_V = 10.0;
constexpr uint32_t DAC_MAX = 4095;

constexpr int DEFAULT_STEP_SEC = 4;
constexpr double TEST_VOLT = 0.6;

std::atomic<bool> g_running{true};

void handle_signal(int) {
    g_running.store(false);
}

bool write_ao(int fd, int ch, double volts) {
    double v = volts;
    if (v < SAFE_VOLT_MIN) v = SAFE_VOLT_MIN;
    if (v > SAFE_VOLT_MAX) v = SAFE_VOLT_MAX;

    uint32_t dac_val = static_cast<uint32_t>((v / DAC_FULL_SCALE_V) * DAC_MAX);
    if (dac_val > DAC_MAX) dac_val = DAC_MAX;

    ixpci_reg_t reg{};
    if (ch == 0) reg.id = AO_CH0_REG;
    else if (ch == 1) reg.id = AO_CH1_REG;
    else if (ch == 2) reg.id = AO_CH2_REG;
    else {
        reg.id = AO_FALLBACK_REG;
        reg.value = (static_cast<uint32_t>(ch) << 12) | dac_val;
        reg.mode = IXPCI_RM_NORMAL;
        return ioctl(fd, IXPCI_WRITE_REG, &reg) >= 0;
    }

    reg.value = dac_val;
    reg.mode = IXPCI_RM_NORMAL;
    return ioctl(fd, IXPCI_WRITE_REG, &reg) >= 0;
}

void set_all_zero(int fd, int max_ch) {
    for (int ch = 0; ch <= max_ch; ++ch) {
        write_ao(fd, ch, 0.0);
    }
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

struct CanSnapshot {
    std::map<uint32_t, std::array<uint8_t, 8>> payload;
};

CanSnapshot read_can_snapshot(int s, int duration_ms) {
    CanSnapshot snap;
    auto start = std::chrono::steady_clock::now();
    while (g_running.load()) {
        struct can_frame frame{};
        int n = read(s, &frame, sizeof(frame));
        if (n > 0) {
            uint32_t id = frame.can_id & CAN_EFF_MASK;
            std::array<uint8_t, 8> data{};
            std::memcpy(data.data(), frame.data, frame.can_dlc);
            snap.payload[id] = data;
        }
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed >= duration_ms) break;
    }
    return snap;
}

std::set<uint32_t> detect_can_changes(const CanSnapshot& before, const CanSnapshot& after) {
    std::set<uint32_t> changed;
    for (const auto& kv : after.payload) {
        auto it = before.payload.find(kv.first);
        if (it == before.payload.end() || it->second != kv.second) {
            changed.insert(kv.first);
        }
    }
    return changed;
}

std::string classify_id(uint32_t id) {
    // Typical motor status IDs: 0x181..0x184
    if (id >= 0x181 && id <= 0x184) {
        int motor = static_cast<int>(id - 0x180);
        return "motor" + std::to_string(motor) + "_status";
    }
    return "unknown";
}

std::string infer_mapping_from_ids(const std::set<uint32_t>& ids) {
    for (uint32_t id : ids) {
        if (id >= 0x181 && id <= 0x184) {
            int motor = static_cast<int>(id - 0x180);
            return "motor" + std::to_string(motor) + "_accelerator";
        }
    }
    return "unknown";
}

void write_json(const std::string& path, const std::map<int, std::string>& mapping) {
    std::ofstream f(path);
    if (!f.is_open()) return;
    f << "{\n";
    bool first = true;
    for (const auto& kv : mapping) {
        if (!first) f << ",\n";
        first = false;
        f << "  \"AO" << kv.first << "\": \"" << kv.second << "\"";
    }
    f << "\n}\n";
}

} // namespace

int main(int argc, char** argv) {
    std::string dev = "/dev/ixpio1";
    std::string can_if = "can0";
    bool manual_mode = false;
    bool monitor_only = false;
    int manual_ch = -1;
    double manual_v = 0.0;
    int step_seconds = DEFAULT_STEP_SEC;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--device=", 0) == 0) {
            dev = arg.substr(9);
        } else if (arg.rfind("--can=", 0) == 0) {
            can_if = arg.substr(6);
        } else if (arg.rfind("--channel=", 0) == 0) {
            manual_mode = true;
            manual_ch = std::stoi(arg.substr(10));
        } else if (arg.rfind("--voltage=", 0) == 0) {
            manual_v = std::stod(arg.substr(10));
        } else if (arg.rfind("--step=", 0) == 0) {
            step_seconds = std::stoi(arg.substr(7));
        } else if (arg == "--monitor-can") {
            monitor_only = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Uso:\n"
                      << "  ./pex_scan --device /dev/ixpio1 --can can0\n"
                      << "  ./pex_scan --channel 2 --voltage 0.6\n"
                      << "  ./pex_scan --monitor-can\n"
                      << "Opciones:\n"
                      << "  --step=4 (segundos)\n";
            return 0;
        }
    }

    if (manual_mode && (manual_ch < 0 || manual_ch > 7)) {
        std::cerr << "[ERR] Canal inválido. Use 0..7\n";
        return 1;
    }
    if (manual_mode && (manual_v < SAFE_VOLT_MIN || manual_v > SAFE_VOLT_MAX)) {
        std::cerr << "[ERR] Voltaje fuera de rango seguro (0.0V - 1.0V)\n";
        return 1;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    int fd = open(dev.c_str(), O_RDWR);
    if (fd < 0) {
        std::cerr << "[ERR] No se pudo abrir " << dev << "\n";
        return 1;
    }

    ixpci_reg_t reg{};
    reg.id = AO_RANGE_REG;
    reg.value = 0; // 0 = 0-10V
    reg.mode = IXPCI_RM_NORMAL;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        std::cerr << "[WARN] No se pudo configurar rango AO\n";
    }

    reg.id = AO_ENABLE_REG;
    reg.value = 0xFF; // AO0..AO7
    reg.mode = IXPCI_RM_NORMAL;
    if (ioctl(fd, IXPCI_WRITE_REG, &reg) < 0) {
        std::cerr << "[ERR] No se pudieron habilitar canales AO\n";
        close(fd);
        return 1;
    }

    int can_fd = open_can(can_if);
    if (can_fd < 0) {
        std::cerr << "[ERR] No se pudo abrir CAN en " << can_if << "\n";
        close(fd);
        return 1;
    }

    set_all_zero(fd, 7);

    if (monitor_only) {
        std::cout << "Monitor CAN activo. Ctrl+C para salir.\n";
        while (g_running.load()) {
            auto snap = read_can_snapshot(can_fd, 500);
            for (const auto& kv : snap.payload) {
                std::cout << "[CAN] ID=0x" << std::hex << kv.first << std::dec << "\n";
            }
        }
        close(can_fd);
        close(fd);
        return 0;
    }

    if (manual_mode) {
        std::cout << "Modo manual: AO" << manual_ch << " = " << manual_v << "V\n";
        std::cout << "Presiona Ctrl+C para detener...\n";
        write_ao(fd, manual_ch, manual_v);
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        set_all_zero(fd, 7);
        close(can_fd);
        close(fd);
        return 0;
    }

    std::map<int, std::string> mapping;

    for (int ch = 0; ch <= 7 && g_running.load(); ++ch) {
        std::cout << "\nTesting AO" << ch << "\n";
        std::cout << "Output = 0.6V. Monitoring CAN...\n";

        auto baseline = read_can_snapshot(can_fd, 1000);

        set_all_zero(fd, 7);
        write_ao(fd, ch, TEST_VOLT);
        std::this_thread::sleep_for(std::chrono::seconds(step_seconds));

        auto during = read_can_snapshot(can_fd, 1000);
        set_all_zero(fd, 7);
        std::this_thread::sleep_for(std::chrono::seconds(step_seconds));

        auto changed = detect_can_changes(baseline, during);
        std::cout << "CAN changes detected: " << changed.size() << "\n";
        for (uint32_t id : changed) {
            std::cout << "  ID 0x" << std::hex << id << std::dec
                      << " (" << classify_id(id) << ")\n";
        }

        mapping[ch] = infer_mapping_from_ids(changed);
    }

    std::cout << "\nDetected mapping:\n";
    for (const auto& kv : mapping) {
        std::cout << "AO" << kv.first << " -> " << kv.second << "\n";
    }

    write_json("ecu_io_map.json", mapping);
    std::cout << "Mapping saved to ecu_io_map.json\n";

    close(can_fd);
    close(fd);
    return 0;
}
