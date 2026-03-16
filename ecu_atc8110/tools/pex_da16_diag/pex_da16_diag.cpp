#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <iostream>
#include <map>
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
#include <sys/ioctl.h>

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

std::atomic<bool> g_running{true};
int g_fd = -1;

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

void can_monitor_thread(const std::string& ifname) {
    int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (s < 0) {
        std::cerr << "[CAN] No se pudo abrir socket CAN\n";
        return;
    }

    struct ifreq ifr{};
    std::snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname.c_str());
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[CAN] No se pudo obtener índice de interfaz\n";
        close(s);
        return;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(s, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[CAN] No se pudo bind\n";
        close(s);
        return;
    }

    std::map<uint32_t, std::array<uint8_t, 8>> last_payload;

    while (g_running.load()) {
        struct can_frame frame{};
        int n = read(s, &frame, sizeof(frame));
        if (n <= 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        uint32_t id = frame.can_id & CAN_EFF_MASK;
        std::array<uint8_t, 8> data{};
        std::memcpy(data.data(), frame.data, frame.can_dlc);

        auto it = last_payload.find(id);
        if (it == last_payload.end() || it->second != data) {
            last_payload[id] = data;
            std::cout << "[CAN] Cambio detectado ID=0x" << std::hex << id << std::dec << "\n";
        }
    }

    close(s);
}

} // namespace

int main(int argc, char** argv) {
    std::string dev = "/dev/ixpio1";
    std::string can_if;
    bool manual_mode = false;
    int manual_ch = -1;
    double manual_v = 0.0;
    int step_seconds = 4;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--device=", 0) == 0) {
            dev = arg.substr(9);
        } else if (arg.rfind("--channel=", 0) == 0) {
            manual_mode = true;
            manual_ch = std::stoi(arg.substr(10));
        } else if (arg.rfind("--voltage=", 0) == 0) {
            manual_v = std::stod(arg.substr(10));
        } else if (arg.rfind("--step=", 0) == 0) {
            step_seconds = std::stoi(arg.substr(7));
        } else if (arg.rfind("--can-if=", 0) == 0) {
            can_if = arg.substr(9);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Uso:\n"
                      << "  ./pex_da16_diag                      (auto AO0..AO7)\n"
                      << "  ./pex_da16_diag --channel=3 --voltage=0.6\n"
                      << "Opciones:\n"
                      << "  --device=/dev/ixpio1\n"
                      << "  --step=4 (segundos)\n"
                      << "  --can-if=can0 (opcional)\n";
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

    g_fd = open(dev.c_str(), O_RDWR);
    if (g_fd < 0) {
        std::cerr << "[ERR] No se pudo abrir " << dev << "\n";
        return 1;
    }

    ixpci_reg_t reg{};
    reg.id = AO_RANGE_REG;
    reg.value = 0; // 0 = 0-10V
    reg.mode = IXPCI_RM_NORMAL;
    if (ioctl(g_fd, IXPCI_WRITE_REG, &reg) < 0) {
        std::cerr << "[WARN] No se pudo configurar rango AO\n";
    }

    reg.id = AO_ENABLE_REG;
    reg.value = 0xFF; // AO0..AO7
    reg.mode = IXPCI_RM_NORMAL;
    if (ioctl(g_fd, IXPCI_WRITE_REG, &reg) < 0) {
        std::cerr << "[ERR] No se pudieron habilitar canales AO\n";
        close(g_fd);
        return 1;
    }

    std::thread can_thread;
    if (!can_if.empty()) {
        can_thread = std::thread(can_monitor_thread, can_if);
    }

    set_all_zero(g_fd, 7);

    if (manual_mode) {
        std::cout << "Modo manual: AO" << manual_ch << " = " << manual_v << "V\n";
        std::cout << "Presiona Ctrl+C para detener...\n";
        write_ao(g_fd, manual_ch, manual_v);
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    } else {
        for (int ch = 0; ch <= 7 && g_running.load(); ++ch) {
            std::cout << "Testing AO" << ch << "\n";
            std::cout << "Output = 0.6V. Observe el motor...\n";
            set_all_zero(g_fd, 7);
            write_ao(g_fd, ch, 0.6);
            std::this_thread::sleep_for(std::chrono::seconds(step_seconds));

            set_all_zero(g_fd, 7);
            std::this_thread::sleep_for(std::chrono::seconds(step_seconds));
        }
    }

    set_all_zero(g_fd, 7);
    close(g_fd);
    g_fd = -1;

    if (can_thread.joinable()) {
        can_thread.join();
    }

    std::cout << "Diagnóstico finalizado.\n";
    return 0;
}
