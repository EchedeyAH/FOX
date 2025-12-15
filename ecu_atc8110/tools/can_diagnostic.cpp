#include "../comunicacion_can/can_manager.hpp"
#include "../comunicacion_can/can_protocol.hpp"
#include "../common/logging.hpp"

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

/**
 * Herramienta de diagnóstico CAN
 * Verifica que la ECU pueda leer mensajes CAN correctamente
 */

std::atomic<bool> running{true};

void signal_handler(int signal) {
    std::cout << "\nDeteniendo diagnóstico...\n";
    running = false;
}

void print_frame(const common::CanFrame& frame, const std::string& direction) {
    std::cout << std::setw(10) << direction << " | ";
    std::cout << "ID: 0x" << std::hex << std::setw(3) << std::setfill('0') << frame.id << std::dec << " | ";
    std::cout << "DLC: " << frame.payload.size() << " | ";
    std::cout << "Data: ";
    
    for (auto byte : frame.payload) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    std::cout << std::dec << "\n";
}

int main(int argc, char** argv) {
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║  DIAGNÓSTICO CAN - ECU ATX-1610       ║\n";
    std::cout << "║  Verificación de lectura de mensajes  ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    // Configurar handler de señal
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // Determinar interfaz a usar
    std::string interface = "emuccan0";
    if (argc > 1) {
        interface = argv[1];
    }
    
    std::cout << "Interfaz CAN: " << interface << "\n";
    std::cout << "Presione Ctrl+C para detener\n\n";
    
    // Crear interfaz CAN
    comunicacion_can::SocketCanInterface can_interface(interface);
    
    // Iniciar CAN
    if (!can_interface.start()) {
        std::cerr << "ERROR: No se pudo iniciar la interfaz CAN\n";
        std::cerr << "Verifique que:\n";
        std::cerr << "  1. La interfaz '" << interface << "' existe\n";
        std::cerr << "  2. Tiene permisos suficientes\n";
        std::cerr << "  3. Ejecutó: sudo ./scripts/setup_can.sh\n";
        return 1;
    }
    
    std::cout << "✓ Interfaz CAN iniciada correctamente\n\n";
    
    // Contadores de mensajes
    struct {
        int bms = 0;
        int motor_cmd[4] = {0, 0, 0, 0};
        int motor_resp[4] = {0, 0, 0, 0};
        int supervisor_hb = 0;
        int supervisor_cmd = 0;
        int unknown = 0;
    } counters;
    
    std::cout << "Monitoreando tráfico CAN...\n";
    std::cout << std::string(80, '=') << "\n";
    std::cout << std::setw(10) << "Dirección" << " | ";
    std::cout << std::setw(12) << "ID CAN" << " | ";
    std::cout << std::setw(6) << "DLC" << " | ";
    std::cout << "Datos\n";
    std::cout << std::string(80, '-') << "\n";
    
    auto last_summary = std::chrono::steady_clock::now();
    
    while (running) {
        // Intentar recibir frame directamente
        auto frame_opt = can_interface.receive();
        
        if (frame_opt.has_value()) {
            auto& frame = frame_opt.value();
            print_frame(frame, "RX");
            
            // Clasificar mensaje
            using namespace comunicacion_can;
            
            if (frame.id == ID_CAN_BMS) {
                counters.bms++;
            } else if (frame.id >= ID_MOTOR_1_CMD && frame.id <= ID_MOTOR_4_CMD) {
                int motor_idx = frame.id - ID_MOTOR_1_CMD;
                counters.motor_cmd[motor_idx]++;
            } else if (frame.id >= ID_MOTOR_1_RESP && frame.id <= ID_MOTOR_4_RESP) {
                int motor_idx = frame.id - ID_MOTOR_1_RESP;
                counters.motor_resp[motor_idx]++;
            } else if (frame.id == ID_SUPERVISOR_HB) {
                counters.supervisor_hb++;
            } else if (frame.id == ID_SUPERVISOR_CMD) {
                counters.supervisor_cmd++;
            } else {
                counters.unknown++;
            }
        }
        
        // Mostrar resumen cada 5 segundos
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_summary).count() >= 5) {
            std::cout << std::string(80, '=') << "\n";
            std::cout << "RESUMEN (últimos 5 segundos):\n";
            std::cout << "  BMS (0x180):        " << counters.bms << " mensajes\n";
            
            for (int i = 0; i < 4; i++) {
                std::cout << "  Motor " << (i+1) << " CMD:       " << counters.motor_cmd[i] << " mensajes\n";
                std::cout << "  Motor " << (i+1) << " RESP:      " << counters.motor_resp[i] << " mensajes\n";
            }
            
            std::cout << "  Supervisor HB:      " << counters.supervisor_hb << " mensajes\n";
            std::cout << "  Supervisor CMD:     " << counters.supervisor_cmd << " mensajes\n";
            std::cout << "  Desconocidos:       " << counters.unknown << " mensajes\n";
            std::cout << std::string(80, '=') << "\n\n";
            
            // Resetear contadores
            counters = {};
            last_summary = now;
        }
        
        // Pequeña pausa para no saturar CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "\n\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║  DIAGNÓSTICO FINALIZADO               ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
    
    can_interface.stop();
    
    return 0;
}
