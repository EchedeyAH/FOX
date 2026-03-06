#pragma once
/**
 * cli_parser.hpp
 * Parser de línea de comandos para el test runner
 * 
 * Argumentos soportados:
 *   --backend=sim|socketcan    Backend de ejecución
 *   --if0=emuccan0             Interfaz CAN primaria
 *   --if1=emuccan1             Interfaz CAN secundaria
 *   --policy=dev|prod          Política de seguridad
 *   --suite=<nombre>           Ejecutar suite específica
 *   --test=<nombre>            Ejecutar test específico
 *   --report                   Generar reportes
 *   --verbose                  Salida detallada
 *   --help                     Mostrar ayuda
 * 
 * Uso:
 *   CLIArgs args;
 *   if (!parse_cli_args(argc, argv, args)) {
 *       return 1;
 *   }
 *   
 *   // Usar args.backend, args.policy, etc.
 */

#include <iostream>
#include <string>
#include <vector>

#include "../backends/ican_backend.hpp"
#include "test_context.hpp"

namespace ecu {
namespace testing {

// ============================================================================
// ESTRUCTURA DE ARGUMENTOS CLI
// ============================================================================

/**
 * Argumentos parseados de la línea de comandos
 */
struct CLIArgs {
    // Backend
    std::string backend = "sim";
    std::string if0 = "emuccan0";
    std::string if1 = "emuccan1";
    
    // Política de seguridad
    SecurityPolicy policy = SecurityPolicy::DEV;
    
    // Filtros de ejecución
    std::string suite_filter;
    std::string test_filter;
    
    // Opciones
    bool generate_report = false;
    bool verbose = false;
    bool help_requested = false;
    
    // Timing (para tests de scheduler)
    uint32_t timing_iterations = 1000;
    uint32_t timing_period_us = 1000;
    
    // Validación
    bool is_valid() const {
        if (backend != "sim" && backend != "socketcan" && 
            backend != "real" && backend != "simulator") {
            return false;
        }
        
        if (policy != SecurityPolicy::DEV && policy != SecurityPolicy::PROD) {
            return false;
        }
        
        return true;
    }
    
    /// Convertir a BackendConfig
    BackendConfig to_backend_config() const {
        BackendConfig config;
        config.backend_type = backend;
        config.if0 = if0;
        config.if1 = if1;
        return config;
    }
};

// ============================================================================
// PARSER CLI
// ============================================================================

/**
 * Parser de argumentos de línea de comandos
 */
class CLIParser {
public:
    /**
     * Parsear argumentos argc/argv
     * @return true si los argumentos son válidos
     */
    static bool parse(int argc, char* argv[], CLIArgs& args) {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            
            // Ayuda
            if (arg == "--help" || arg == "-h") {
                args.help_requested = true;
                return true;
            }
            
            // Backend
            else if (arg.rfind("--backend=", 0) == 0) {
                args.backend = arg.substr(10);
            }
            
            // Interfaz CAN primaria
            else if (arg.rfind("--if0=", 0) == 0) {
                args.if0 = arg.substr(6);
            }
            
            // Interfaz CAN secundaria
            else if (arg.rfind("--if1=", 0) == 0) {
                args.if1 = arg.substr(6);
            }
            
            // Política de seguridad
            else if (arg.rfind("--policy=", 0) == 0) {
                std::string policy_str = arg.substr(9);
                if (policy_str == "dev" || policy_str == "development") {
                    args.policy = SecurityPolicy::DEV;
                }
                else if (policy_str == "prod" || policy_str == "production") {
                    args.policy = SecurityPolicy::PROD;
                }
                else {
                    std::cerr << "Política desconocida: " << policy_str << "\n";
                    return false;
                }
            }
            
            // Suite
            else if (arg.rfind("--suite=", 0) == 0) {
                args.suite_filter = arg.substr(8);
            }
            
            // Test
            else if (arg.rfind("--test=", 0) == 0) {
                args.test_filter = arg.substr(7);
            }
            
            // Reporte
            else if (arg == "--report") {
                args.generate_report = true;
            }
            
            // Verbose
            else if (arg == "--verbose" || arg == "-v") {
                args.verbose = true;
            }
            
            // Timing iterations
            else if (arg.rfind("--timing-iterations=", 0) == 0) {
                try {
                    args.timing_iterations = std::stoul(arg.substr(20));
                }
                catch (...) {
                    std::cerr << "Valor inválido para --timing-iterations\n";
                    return false;
                }
            }
            
            // Timing period
            else if (arg.rfind("--timing-period=", 0) == 0) {
                try {
                    args.timing_period_us = std::stoul(arg.substr(16));
                }
                catch (...) {
                    std::cerr << "Valor inválido para --timing-period\n";
                    return false;
                }
            }
            
            // Desconocido
            else {
                std::cerr << "Argumento desconocido: " << arg << "\n";
                return false;
            }
        }
        
        return args.is_valid();
    }
    
    /**
     * Mostrar ayuda del programa
     */
    static void print_usage(const char* prog) {
        std::cout << "Uso: " << prog << " [opciones]\n\n";
        
        std::cout << "Opciones de Backend:\n";
        std::cout << "  --backend=<tipo>     Backend de ejecución: sim (default) o socketcan\n";
        std::cout << "  --if0=<nombre>      Interfaz CAN primaria (default: emuccan0)\n";
        std::cout << "  --if1=<nombre>      Interfaz CAN secundaria (default: emuccan1)\n\n";
        
        std::cout << "Opciones de Política:\n";
        std::cout << "  --policy=<tipo>     Política de seguridad: dev (default) o prod\n";
        std::cout << "                      DEV: recovery automático de SAFE_STOP\n";
        std::cout << "                      PROD: requiere ACK y condiciones estables\n\n";
        
        std::cout << "Opciones de Ejecución:\n";
        std::cout << "  --suite=<nombre>    Ejecutar solo suite especificada\n";
        std::cout << "  --test=<nombre>     Ejecutar solo test específico\n\n";
        
        std::cout << "Opciones de Reporte:\n";
        std::cout << "  --report            Generar reportes JSON y MD\n";
        std::cout << "  --verbose, -v       Salida detallada\n\n";
        
        std::cout << "Opciones de Timing:\n";
        std::cout << "  --timing-iterations=N  Iteraciones para tests de timing (default: 1000)\n";
        std::cout << "  --timing-period=US      Período base en us (default: 1000)\n\n";
        
        std::cout << "Opciones Generales:\n";
        std::cout << "  --help, -h         Mostrar esta ayuda\n\n";
        
        std::cout << "Ejemplos:\n";
        std::cout << "  " << prog << "                           # Todos los tests en SIM\n";
        std::cout << "  " << prog << " --backend=sim              # Modo simulación\n";
        std::cout << "  " << prog << " --backend=socketcan --if0=emuccan0  # ECU real\n";
        std::cout << "  " << prog << " --policy=prod             # Modo producción\n";
        std::cout << "  " << prog << " --suite=MotorTimeoutSuite # Suite específica\n";
        std::cout << "  " << prog << " --test=MT_01              # Test específico\n";
        std::cout << "  " << prog << " --report                  # Con reportes\n";
        
        std::cout << "\n\nSuites disponibles:\n";
        std::cout << "  - MotorTimeoutSuite\n";
        std::cout << "  - VoltageSuite\n";
        std::cout << "  - SocSuite\n";
        std::cout << "  - TemperatureSuite\n";
        std::cout << "  - SchedulerSuite\n";
        std::cout << "  - IntegrationSuite\n";
        std::cout << "  - CanCommunicationSuite\n";
        std::cout << "  - HmiIntegrationSuite\n";
        
        std::cout << "\n\nPolíticas de Seguridad:\n";
        std::cout << "  DEV (default):\n";
        std::cout << "    - Recovery automático de SAFE_STOP permitido\n";
        std::cout << "    - No requiere ACK\n";
        std::cout << "    - Timeout de recovery: 5s\n\n";
        
        std::cout << "  PROD:\n";
        std::cout << "    - Recovery automático NO permitido\n";
        std::cout << "    - Requiere ACK explícito\n";
        std::cout << "    - Requiere condiciones estables (2s mínimo)\n";
        std::cout << "    - Timeout de recovery: 30s\n";
    }
    
    /**
     * Mostrar banner del test runner
     */
    static void print_banner() {
        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║           ECU ATC8110 TEST RUNNER v2.0                         ║\n";
        std::cout << "║           Framework de Validación de Firmware                   ║\n";
        std::cout << "║           Soporta: SIM y SocketCAN (ECU Real)                  ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n";
        std::cout << "\n";
    }
};

} // namespace testing
} // namespace ecu
