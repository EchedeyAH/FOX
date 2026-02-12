#include "adquisicion_datos/sensor_manager.hpp"
#include "common/logging.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <csignal>

volatile sig_atomic_t g_running = 1;

void signal_handler(int) {
    g_running = 0;
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "========================================" << std::endl;
    std::cout << " Test de Entradas Analógicas PEX-1202L" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Inicializar sensor manager
    adquisicion_datos::SensorManager sensores;
    
    std::cout << "Inicializando PEX-1202L..." << std::endl;
    if (!sensores.start()) {
        std::cerr << "ERROR: No se pudo inicializar PEX-1202L" << std::endl;
        std::cerr << "Verifica que /dev/ixpci1 existe y tienes permisos (sudo)" << std::endl;
        return 1;
    }
    
    std::cout << "✓ PEX-1202L inicializado correctamente" << std::endl;
    std::cout << std::endl;
    std::cout << "Leyendo sensores cada 200ms..." << std::endl;
    std::cout << "Presiona Ctrl+C para detener" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Encabezado de tabla
    std::cout << std::left 
              << std::setw(28) << "SENSOR"
              << std::setw(12) << "VALOR"
              << std::setw(15) << "RANGO"
              << std::endl;
    std::cout << std::string(55, '-') << std::endl;
    
    int sample_count = 0;
    
    while (g_running) {
        // Leer sensores
        auto samples = sensores.poll();
        
        // Limpiar pantalla cada 10 muestras (cada 2 segundos)
        if (sample_count % 10 == 0 && sample_count > 0) {
            std::cout << "\033[2J\033[1;1H";  // Clear screen
            std::cout << "========================================" << std::endl;
            std::cout << " Test de Entradas Analógicas PEX-1202L" << std::endl;
            std::cout << " Muestra #" << sample_count << std::endl;
            std::cout << "========================================" << std::endl;
            std::cout << std::endl;
            std::cout << std::left 
                      << std::setw(28) << "SENSOR"
                      << std::setw(12) << "VALOR"
                      << std::setw(15) << "RANGO"
                      << std::endl;
            std::cout << std::string(55, '-') << std::endl;
        }
        
        // Mostrar cada sensor
        for (const auto& sample : samples) {
            std::string range;
            std::string bar;
            
            if (sample.name == "acelerador" || sample.name == "freno") {
                // Normalizado 0-1
                range = "0.0 - 1.0";
                int bar_len = static_cast<int>(sample.value * 30);
                bar = std::string(bar_len, '=');
                if (bar_len > 0) bar += ">";
            } else if (sample.name == "corriente_eje_d" || sample.name == "corriente_eje_t") {
                // Bipolar ±5V
                range = "-5.0 - +5.0 V";
                int bar_len = static_cast<int>((sample.value + 5.0) / 10.0 * 30);
                bar = std::string(std::max(0, bar_len), '=');
                if (bar_len > 0) bar += ">";
            } else {
                // Otros (0-5V típicamente)
                range = "0.0 - 5.0 V";
                int bar_len = static_cast<int>(sample.value / 5.0 * 30);
                bar = std::string(std::max(0, bar_len), '=');
                if (bar_len > 0) bar += ">";
            }
            
            std::cout << std::left
                      << std::setw(28) << sample.name
                      << std::setw(12) << std::fixed << std::setprecision(3) << sample.value
                      << std::setw(15) << range
                      << " " << bar
                      << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "Muestra #" << ++sample_count << " | Presiona Ctrl+C para salir" << std::endl;
        
        // Esperar 200ms
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test finalizado. Total de muestras: " << sample_count << std::endl;
    std::cout << "========================================" << std::endl;
    
    sensores.stop();
    return 0;
}
