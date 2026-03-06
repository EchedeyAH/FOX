/**
 * ecu_tests_main.cpp
 * Test Runner principal para el sistema ECU ATC8110
 * 
 * Uso:
 *   ./ecu_tests [--suite=<nombre>] [--test=<nombre>] [--report]
 * 
 * Ejemplos:
 *   ./ecu_tests                    # Ejecutar todos los tests
 *   ./ecu_tests --suite=MotorTimeoutSuite  # Solo Motor Timeout
 *   ./ecu_tests --test=MT_01      # Solo test específico
 *   ./ecu_tests --report          # Generar reportes
 */

#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "ecu_test_framework.hpp"
#include "test_suites.hpp"

using namespace ecu::testing;

// ============================================================================
// CONFIGURACIÓN GLOBAL
// ============================================================================

// Instancias globales de los módulos del sistema
static common::SystemModeManager g_mode_manager;
static common::MotorTimeoutManager g_motor_timeout;
static common::CanSimulator g_can_simulator;
static common::TelemetryPublisher g_telemetry_publisher;

// ============================================================================
// IMPLEMENTACIONES DE REFERENCIAS GLOBALES
// ============================================================================

namespace common {

// Referencias a las instancias globales para los tests
SystemModeManager& get_system_mode_manager() {
    return g_mode_manager;
}

MotorTimeoutManager& get_motor_timeout_detector() {
    return g_motor_timeout;
}

CanSimulator& get_can_simulator() {
    return g_can_simulator;
}

TelemetryPublisher& get_telemetry_publisher() {
    return g_telemetry_publisher;
}

} // namespace common

// ============================================================================
// HELPERS
// ============================================================================

void print_usage(const char* prog) {
    std::cout << "Uso: " << prog << " [opciones]\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  --help              Mostrar esta ayuda\n";
    std::cout << "  --suite=<nombre>    Ejecutar solo suite especificada\n";
    std::cout << "  --test=<nombre>     Ejecutar solo test especificado\n";
    std::cout << "  --report            Generar reportes JSON y MD\n";
    std::cout << "  --verbose           Salida detallada\n\n";
    std::cout << "Suites disponibles:\n";
    std::cout << "  - MotorTimeoutSuite\n";
    std::cout << "  - VoltageSuite\n";
    std::cout << "  - SocSuite\n";
    std::cout << "  - TemperatureSuite\n";
    std::cout << "  - SchedulerSuite\n";
    std::cout << "  - IntegrationSuite\n";
    std::cout << "  - CanCommunicationSuite\n";
    std::cout << "  - HmiIntegrationSuite\n";
}

void print_header() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║          ECU ATC8110 TEST RUNNER v1.0                 ║\n";
    std::cout << "║          Sistema de Validación de Firmware             ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    // Parsear argumentos
    std::string suite_filter;
    std::string test_filter;
    bool generate_report = false;
    bool verbose = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        }
        else if (arg.substr(0, 8) == "--suite=") {
            suite_filter = arg.substr(8);
        }
        else if (arg.substr(0, 7) == "--test=") {
            test_filter = arg.substr(7);
        }
        else if (arg == "--report") {
            generate_report = true;
        }
        else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        }
        else {
            std::cerr << "Argumento desconocido: " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }
    
    print_header();
    
    // Crear framework
    ECUTestFramework framework;
    
    // Configurar referencias a módulos
    framework.set_mode_manager(&g_mode_manager);
    framework.set_simulator(&g_can_simulator);
    
    // Añadir todas las suites
    framework.add_suite<MotorTimeoutSuite>();
    framework.add_suite<VoltageSuite>();
    framework.add_suite<SocSuite>();
    framework.add_suite<TemperatureSuite>();
    framework.add_suite<SchedulerSuite>();
    framework.add_suite<IntegrationSuite>();
    framework.add_suite<CanCommunicationSuite>();
    framework.add_suite<HmiIntegrationSuite>();
    
    // Ejecutar tests
    if (!suite_filter.empty()) {
        std::cout << "Ejecutando suite: " << suite_filter << "\n\n";
        framework.run_suite(suite_filter);
    }
    else if (!test_filter.empty()) {
        std::cout << "Ejecutando test: " << test_filter << "\n\n";
        framework.run_test(test_filter);
    }
    else {
        std::cout << "Ejecutando todas las suites de tests...\n\n";
        framework.run();
    }
    
    // Generar reportes
    if (generate_report) {
        std::cout << "\nGenerando reportes...\n";
        framework.generate_report();
    }
    
    // Resultado final
    int exit_code = (framework.get_fail_count() > 0) ? 1 : 0;
    
    std::cout << "\n════════════════════════════════════════════════════════\n";
    if (exit_code == 0) {
        std::cout << "  ✅ TODOS LOS TESTS PASARON - BUILD VALIDADO\n";
    } else {
        std::cout << "  ❌ ALGUNOS TESTS FALLARON - REVISAR RESULTADOS\n";
    }
    std::cout << "════════════════════════════════════════════════════════\n\n";
    
    return exit_code;
}
