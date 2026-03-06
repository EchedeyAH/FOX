/**
 * ecu_tests_main_v2.cpp
 * Test Runner principal v2.0 para el sistema ECU ATC8110
 * 
 * Características:
 * - Backends SIM y SocketCAN (ECU real)
 * - Políticas de seguridad DEV/PROD
 * - Métricas de timing
 * - Reportes enriquecidos
 * 
 * Uso:
 *   ./ecu_tests [--backend=sim|socketcan] [--policy=dev|prod]
 *                [--suite=<nombre>] [--test=<nombre>] [--report]
 * 
 * Ejemplos:
 *   ./ecu_tests                          # Todos los tests en SIM (DEV)
 *   ./ecu_tests --backend=sim            # Modo simulación
 *   ./ecu_tests --backend=socketcan --if0=emuccan0  # ECU real
 *   ./ecu_tests --policy=prod           # Modo producción
 *   ./ecu_tests --suite=SchedulerTimingSuite --timing-iterations=5000
 */

#include <csignal>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <sys/utsname.h>

#include "ecu_test_framework.hpp"
#include "test_suites.hpp"
#include "test_suites_v2.hpp"
#include "framework/cli_parser.hpp"
#include "framework/test_context.hpp"
#include "framework/timing_metrics.hpp"
#include "backends/backend_factory.hpp"

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
// SYSTEM INFO COLLECTOR
// ============================================================================

/**
 * Recolecta información del sistema Linux
 */
SystemInfo collect_system_info() {
    SystemInfo info;
    
    struct utsname uname_data;
    if (uname(&uname_data) == 0) {
        info.kernel_version = uname_data.release;
        info.architecture = uname_data.machine;
    }
    
    info.cpu_count = std::thread::hardware_concurrency();
    
    // Detectar preempt_rt
    std::ifstream cmdline("/proc/cmdline");
    std::string line;
    if (std::getline(cmdline, line)) {
        info.preempt_rt_detected = (line.find("PREEMPT_RT") != std::string::npos ||
                                    line.find("preempt_rt") != std::string::npos);
    }
    
    // Verificar permisos SCHED_FIFO
    info.sched_fifo_available = true;  // Asumimos disponible en Linux
    
    // Verificar interfaces CAN
    std::ifstream can_dev("/sys/class/net/");
    bool has_can = false;
    std::string ifname;
    while (std::getline(can_dev, ifname)) {
        if (ifname.find("can") == 0 || ifname.find("emucc") == 0) {
            has_can = true;
            if (info.can_interface_name.empty()) {
                info.can_interface_name = ifname;
            }
        }
    }
    info.can_interface_exists = has_can;
    
    return info;
}

// ============================================================================
// ENHANCED REPORT GENERATOR
// ============================================================================

/**
 * Genera reporte JSON enriquecido con metadata del sistema
 */
void generate_enhanced_json_report(ECUTestFramework& framework, 
                                   const CLIArgs& args,
                                   const SystemInfo& sys_info,
                                   const std::string& filename = "test_report.json") {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo crear reporte JSON: " << filename << std::endl;
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    file << "{\n";
    
    // Metadata del framework
    file << "  \"framework\": {\n";
    file << "    \"version\": \"2.0.0\",\n";
    file << "    \"name\": \"ECU ATC8110 Test Framework\"\n";
    file << "  },\n";
    
    // Configuración de ejecución
    file << "  \"execution_config\": {\n";
    file << "    \"backend\": \"" << args.backend << "\",\n";
    file << "    \"policy\": \"" << (args.policy == SecurityPolicy::DEV ? "DEV" : "PROD") << "\",\n";
    file << "    \"if0\": \"" << args.if0 << "\",\n";
    file << "    \"if1\": \"" << args.if1 << "\",\n";
    file << "    \"timing_iterations\": " << args.timing_iterations << ",\n";
    file << "    \"timing_period_us\": " << args.timing_period_us << "\n";
    file << "  },\n";
    
    // Información del sistema
    file << "  \"system_info\": {\n";
    file << "    \"kernel_version\": \"" << sys_info.kernel_version << "\",\n";
    file << "    \"architecture\": \"" << sys_info.architecture << "\",\n";
    file << "    \"cpu_count\": " << sys_info.cpu_count << ",\n";
    file << "    \"preempt_rt_detected\": " << (sys_info.preempt_rt_detected ? "true" : "false") << ",\n";
    file << "    \"sched_fifo_available\": " << (sys_info.sched_fifo_available ? "true" : "false") << ",\n";
    file << "    \"can_interface_exists\": " << (sys_info.can_interface_exists ? "true" : "false") << ",\n";
    file << "    \"can_interface_name\": \"" << sys_info.can_interface_name << "\",\n";
    file << "    \"test_timestamp_ms\": " << now_ms << "\n";
    file << "  },\n";
    
    // Summary
    file << "  \"summary\": {\n";
    file << "    \"total_tests\": " << framework.get_total_tests() << ",\n";
    file << "    \"passed\": " << framework.get_pass_count() << ",\n";
    file << "    \"failed\": " << framework.get_fail_count() << ",\n";
    file << "    \"skipped\": " << framework.get_skip_count() << "\n";
    file << "  },\n";
    
    // Results
    file << "  \"results\": [\n";
    const auto& results = framework.get_results();
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& r = results[i];
        file << "    {\n";
        file << "      \"name\": \"" << r.name << "\",\n";
        file << "      \"suite\": \"" << r.suite << "\",\n";
        
        std::string result_str;
        switch (r.result) {
            case TestResult::PASS: result_str = "PASS"; break;
            case TestResult::FAIL: result_str = "FAIL"; break;
            case TestResult::SKIPPED: result_str = "SKIPPED"; break;
            case TestResult::TIMEOUT: result_str = "TIMEOUT"; break;
            default: result_str = "NOT_RUN"; break;
        }
        
        file << "      \"result\": \"" << result_str << "\",\n";
        file << "      \"message\": \"" << r.message << "\",\n";
        file << "      \"duration_us\": " << r.duration_us_ << ",\n";
        file << "      \"timestamp_ms\": " << r.start_timestamp_ms << "\n";
        file << "    }";
        if (i < results.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    
    file << "}\n";
    
    file.close();
    std::cout << "\n[REPORT] JSON: " << filename << std::endl;
}

/**
 * Genera reporte Markdown enriquecido
 */
void generate_enhanced_markdown_report(ECUTestFramework& framework,
                                       const CLIArgs& args,
                                       const SystemInfo& sys_info,
                                       const std::string& filename = "test_report.md") {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo crear reporte MD: " << filename << std::endl;
        return;
    }
    
    file << "# ECU ATC8110 Test Report\n\n";
    
    // Metadata
    file << "## Configuration\n\n";
    file << "| Parameter | Value |\n";
    file << "|-----------|-------|\n";
    file << "| Backend | " << args.backend << " |\n";
    file << "| Policy | " << (args.policy == SecurityPolicy::DEV ? "DEV" : "PROD") << " |\n";
    file << "| Interface 0 | " << args.if0 << " |\n";
    file << "| Interface 1 | " << args.if1 << " |\n\n";
    
    // System Info
    file << "## System Information\n\n";
    file << "| Parameter | Value |\n";
    file << "|-----------|-------|\n";
    file << "| Kernel | " << sys_info.kernel_version << " |\n";
    file << "| Architecture | " << sys_info.architecture << " |\n";
    file << "| CPU Count | " << sys_info.cpu_count << " |\n";
    file << "| PREEMPT_RT | " << (sys_info.preempt_rt_detected ? "Yes" : "No") << " |\n";
    file << "| SCHED_FIFO | " << (sys_info.sched_fifo_available ? "Available" : "N/A") << " |\n";
    file << "| CAN Interface | " << sys_info.can_interface_name << " |\n\n";
    
    // Summary
    file << "## Summary\n\n";
    file << "| Metric | Value |\n";
    file << "|--------|-------|\n";
    file << "| Total Tests | " << framework.get_total_tests() << " |\n";
    file << "| Passed | " << framework.get_pass_count() << " |\n";
    file << "| Failed | " << framework.get_fail_count() << " |\n";
    file << "| Skipped | " << framework.get_skip_count() << " |\n\n";
    
    // Results table
    file << "## Results\n\n";
    file << "| Test | Suite | Result | Duration |\n";
    file << "|------|-------|--------|----------|\n";
    
    for (const auto& r : framework.get_results()) {
        std::string status = (r.result == TestResult::PASS) ? "✅ PASS" : "❌ FAIL";
        file << "| " << r.name << " | " << r.suite << " | " 
             << status << " | " << (r.duration_us_ / 1000) << "ms |\n";
    }
    
    file.close();
    std::cout << "[REPORT] Markdown: " << filename << std::endl;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    // Parsear argumentos
    CLIArgs args;
    
    if (!CLIParser::parse(argc, argv, args)) {
        std::cerr << "\nError: Argumentos inválidos\n\n";
        CLIParser::print_usage(argv[0]);
        return 2;  // SKIPPED/ENV error
    }
    
    if (args.help_requested) {
        CLIParser::print_usage(argv[0]);
        return 0;
    }
    
    CLIParser::print_banner();
    
    // Mostrar configuración
    std::cout << "[CONFIG] Backend: " << args.backend << std::endl;
    std::cout << "[CONFIG] Policy: " << (args.policy == SecurityPolicy::DEV ? "DEV" : "PROD") << std::endl;
    std::cout << "[CONFIG] Interface 0: " << args.if0 << std::endl;
    std::cout << "[CONFIG] Interface 1: " << args.if1 << std::endl;
    
    // Recolectar información del sistema
    SystemInfo sys_info = collect_system_info();
    
    std::cout << "[INFO] Kernel: " << sys_info.kernel_version << std::endl;
    std::cout << "[INFO] CPUs: " << sys_info.cpu_count << std::endl;
    std::cout << "[INFO] PREEMPT_RT: " << (sys_info.preempt_rt_detected ? "Yes" : "No") << std::endl;
    std::cout << "[INFO] CAN Interface: " << sys_info.can_interface_name << std::endl;
    
    // Crear framework
    ECUTestFramework framework;
    TestContext test_ctx;
    BackendManager backend_mgr;
    
    // Configurar referencias a módulos
    framework.set_mode_manager(&g_mode_manager);
    framework.set_simulator(&g_can_simulator);
    
    // Inicializar contexto y backend
    BackendConfig backend_config = args.to_backend_config();
    test_ctx.initialize(backend_config, args.policy);
    
    if (!backend_mgr.initialize(backend_config)) {
        test_ctx.should_skip = true;
        test_ctx.skip_reason = "Backend initialization failed";
    } else if (!backend_mgr.start()) {
        test_ctx.should_skip = true;
        test_ctx.skip_reason = "Backend start failed";
    }
    
    test_ctx.attach_backend(backend_mgr.get());
    framework.set_test_context(&test_ctx);
    
    // Añadir todas las suites
    framework.add_suite<MotorTimeoutSuiteV2>();
    framework.add_suite<VoltageSuite>();
    framework.add_suite<SocSuite>();
    framework.add_suite<TemperatureSuite>();
    framework.add_suite<SchedulerTimingSuite>();  // Nueva suite de timing
    framework.add_suite<IntegrationSuite>();
    framework.add_suite<CanCommunicationSuite>();
    framework.add_suite<HmiIntegrationSuiteV2>();  // Suite HMI mejorada
    
    // Ejecutar tests
    if (!args.suite_filter.empty()) {
        std::cout << "\n[SUITE] Ejecutando: " << args.suite_filter << "\n\n";
        framework.run_suite(args.suite_filter);
    }
    else if (!args.test_filter.empty()) {
        std::cout << "\n[TEST] Ejecutando: " << args.test_filter << "\n\n";
        framework.run_test(args.test_filter);
    }
    else {
        std::cout << "\n[RUN] Ejecutando todas las suites...\n\n";
        framework.run();
    }
    
    // Generar reportes
    if (args.generate_report) {
        std::cout << "\n[REPORT] Generando reportes...\n";
        generate_enhanced_json_report(framework, args, sys_info);
        generate_enhanced_markdown_report(framework, args, sys_info);
        framework.generate_report();  // Incluye metadata extendida desde TestContext
    }
    
    // Determinar exit code
    int exit_code = 0;
    if (framework.get_fail_count() > 0) {
        exit_code = 1;  // FAIL
    }
    else if (framework.get_skip_count() == framework.get_total_tests()) {
        exit_code = 2;  // SKIPPED/ENV
    }
    else if (framework.get_total_tests() == 0) {
        exit_code = 2;  // No tests ran
    }
    
    // Resumen final
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    
    if (exit_code == 0) {
        std::cout << "  ✅ TODOS LOS TESTS PASARON - BUILD VALIDADO\n";
    }
    else if (exit_code == 1) {
        std::cout << "  ❌ ALGUNOS TESTS FALLARON - REVISAR RESULTADOS\n";
    }
    else {
        std::cout << "  ⚠️  TESTS NO EJECUTADOS O ENTORNO NO DISPONIBLE\n";
    }
    
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "\n";
    std::cout << "Ejecutar con:\n";
    std::cout << "  --backend=sim          Modo simulación\n";
    std::cout << "  --backend=socketcan    ECU real\n";
    std::cout << "  --policy=prod          Modo producción\n";
    std::cout << "  --report               Generar reportes\n\n";
    
    // Detener backend
    backend_mgr.stop();
    
    return exit_code;
}
