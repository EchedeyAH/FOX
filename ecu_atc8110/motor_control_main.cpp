#include "logica_sistema/state_machine.hpp"
#include "common/logging.hpp"
#include <csignal>
#include <chrono>
#include <thread>
#include <iostream>

volatile sig_atomic_t g_running = 1;

void signal_handler(int) {
    g_running = 0;
}

int main()
{
    // Configurar handler de señales
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "========================================" << std::endl;
    std::cout << " ECU ATC8110 - Control de Motores FOX" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    LOG_INFO("MAIN", "Inicializando sistema...");
    
    // Inicializar sistema
    logica_sistema::StateMachine fsm;
    fsm.start();
    
    std::cout << std::endl;
    std::cout << "Sistema iniciado correctamente." << std::endl;
    std::cout << std::endl;
    std::cout << "INSTRUCCIONES:" << std::endl;
    std::cout << "1. Pisa el FRENO para iniciar la secuencia de arranque" << std::endl;
    std::cout << "2. Suelta el freno una vez activados los motores" << std::endl;
    std::cout << "3. Presiona ACELERADOR para controlar los motores" << std::endl;
    std::cout << "4. Presiona Ctrl+C para detener el sistema" << std::endl;
    std::cout << std::endl;
    std::cout << "Esperando entrada..." << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Loop principal a 20Hz (50ms)
    int cycle_count = 0;
    while (g_running) {
        fsm.step();
        
        // Log periódico cada 2 segundos
        if (++cycle_count % 40 == 0) {
            LOG_INFO("MAIN", "Sistema operando... (Ctrl+C para detener)");
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    LOG_INFO("MAIN", "Apagando sistema...");
    std::cout << "Sistema detenido correctamente." << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
