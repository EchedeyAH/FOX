#pragma once
/**
 * test_context.hpp
 * Contexto enriquecido para tests
 * 
 * Contiene toda la información de entorno, backend, política y métricas
 * disponible para cada test.
 * 
 * Uso:
 *   TestContext ctx;
 *   ctx.backend->send(frame);
 *   ctx.policy->require_safe_stop_ack();
 */

#include "../backends/ican_backend.hpp"
#include "clock_provider.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#ifdef __linux__
#include <dirent.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

namespace ecu {
namespace testing {

// ============================================================================
// TIPOS DE TEST
// ============================================================================

/**
 * Categoría de test según sus requisitos de ejecución
 */
enum class TestCategory {
    LOGIC,      // Test lógico, determinista, no depende de timing real
    TIMING,     // Test de timing, mide jitter, deadline misses
    INTEGRATION, // Test de integración, puede usar ambos
    REAL_ONLY   // Solo ejecutable en ECU real
};

/**
 * Backend requerido para el test
 */
enum class RequiredBackend {
    SIM_ONLY,   // Solo funciona con simulator
    REAL_ONLY,  // Solo funciona con hardware real
    BOTH        // Funciona en ambos
};

/**
 * Metadata del test para decisiones de ejecución
 */
struct TestMetadata {
    TestCategory category = TestCategory::LOGIC;
    RequiredBackend required_backend = RequiredBackend::BOTH;
    bool requires_sched_fifo = false;
    bool requires_can = false;
    bool requires_hmi_socket = false;
};

// ============================================================================
// POLÍTICAS DE SEGURIDAD
// ============================================================================

/**
 * Políticas de seguridad configurables
 */
enum class SecurityPolicy {
    DEV,    // Desarrollo: recovery automático de SAFE_STOP
    PROD    // Producción: SAFE_STOP requiere reset explícito
};

/**
 * Resultado de recovery
 */
enum class RecoveryResult {
    SUCCESS,
    TIMEOUT,
    FAILED,
    NOT_NEEDED
};

/**
 * Interfaz para políticas de seguridad
 */
class ISecurityPolicy {
public:
    virtual ~ISecurityPolicy() = default;
    
    /// Obtener tipo de política
    virtual SecurityPolicy get_type() const = 0;
    
    /// Verificar si permite recovery automático
    virtual bool allows_auto_recovery() const = 0;
    
    /// Verificar si SAFE_STOP requiere ACK
    virtual bool requires_safe_stop_ack() const = 0;
    
    /// Verificar si requiere condiciones estables para recovery
    virtual bool requires_stable_conditions() const = 0;
    
    /// Obtener timeout para recovery (ms)
    virtual uint32_t get_recovery_timeout_ms() const = 0;
    
    /// Obtener tiempo mínimo estable para recovery (ms)
    virtual uint32_t get_stable_time_ms() const = 0;
    
    /// Validar recovery - Called antes de permitir transición desde SAFE_STOP
    virtual bool validate_recovery() = 0;
    
    /// Nombre de la política
    virtual const char* name() const = 0;
};

/**
 * Política DEV: Recovery automático permitido
 */
class DevSecurityPolicy : public ISecurityPolicy {
public:
    DevSecurityPolicy() = default;
    
    SecurityPolicy get_type() const override { return SecurityPolicy::DEV; }
    bool allows_auto_recovery() const override { return true; }
    bool requires_safe_stop_ack() const override { return false; }
    bool requires_stable_conditions() const override { return false; }
    uint32_t get_recovery_timeout_ms() const override { return 5000; }
    uint32_t get_stable_time_ms() const override { return 100; }
    
    bool validate_recovery() override { 
        // En DEV, siempre permite recovery
        return true; 
    }
    
    const char* name() const override { return "DEV"; }
};

/**
 * Política PROD: Restricciones estrictas
 */
class ProdSecurityPolicy : public ISecurityPolicy {
public:
    ProdSecurityPolicy() : stable_duration_ms_(0), last_check_time_(0) {}
    
    SecurityPolicy get_type() const override { return SecurityPolicy::PROD; }
    bool allows_auto_recovery() const override { return false; }
    bool requires_safe_stop_ack() const override { return true; }
    bool requires_stable_conditions() const override { return true; }
    uint32_t get_recovery_timeout_ms() const override { return 30000; }
    uint32_t get_stable_time_ms() const override { return 2000; }
    
    bool validate_recovery() override {
        // En PROD, verifica condiciones estables
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count() - last_check_time_;
        
        return elapsed >= stable_duration_ms_;
    }
    
    void record_stable_condition() {
        last_check_time_ = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    const char* name() const override { return "PROD"; }
    
private:
    int64_t stable_duration_ms_;
    int64_t last_check_time_;
};

// ============================================================================
// INFORMACIÓN DEL SISTEMA
// ============================================================================

/**
 * Información del sistema Linux
 */
struct SystemInfo {
    std::string kernel_version;
    std::string architecture;
    std::string cpu_model;
    int cpu_count = 0;
    bool preempt_rt_detected = false;
    bool sched_fifo_available = false;
    bool can_interface_exists = false;
    std::string can_interface_name;
    
    // Permisos
    bool has_cap_sys_nice = false;
    bool has_root = false;

    // HMI socket
    bool hmi_socket_exists = false;
    
    // Timestamps
    uint64_t boot_time_ms = 0;
    uint64_t test_start_time_ms = 0;
};

/**
 * Información del entorno de ejecución
 */
struct EnvironmentInfo {
    std::string working_directory;
    std::string hostname;
    std::string user;
    uint32_t available_memory_kb = 0;
    
    // Variables de entorno relevantes
    std::string ecu_config_path;
    std::string can_device_primary;
    std::string can_device_secondary;
};

// ============================================================================
// TEST CONTEXT
// ============================================================================

/**
 * Contexto completo para ejecución de tests
 */
struct TestContext {
    // ─────────────────────────────────────────────────────────────────────
    // BACKEND
    // ─────────────────────────────────────────────────────────────────────
    
    /// Backend CAN activo
    ICanBackend* backend = nullptr;
    
    /// Configuración del backend
    BackendConfig backend_config;
    
    // ─────────────────────────────────────────────────────────────────────
    // RELOJ
    // ─────────────────────────────────────────────────────────────────────
    
    /// Gestor de tiempo
    ClockManager clock_manager;
    
    // ─────────────────────────────────────────────────────────────────────
    // POLÍTICA DE SEGURIDAD
    // ─────────────────────────────────────────────────────────────────────
    
    /// Política de seguridad activa
    std::unique_ptr<ISecurityPolicy> security_policy;
    
    // ─────────────────────────────────────────────────────────────────────
    // INFORMACIÓN DEL SISTEMA
    // ─────────────────────────────────────────────────────────────────────
    
    /// Información del sistema Linux
    SystemInfo system_info;
    
    /// Información del entorno
    EnvironmentInfo env_info;
    
    // ─────────────────────────────────────────────────────────────────────
    // METADATOS DE EJECUCIÓN
    // ─────────────────────────────────────────────────────────────────────
    
    /// Categoría del test actual
    TestCategory test_category = TestCategory::LOGIC;
    
    /// Backend requerido para el test
    RequiredBackend required_backend = RequiredBackend::BOTH;
    
    /// Nombre del test actual
    std::string current_test_name;
    
    /// Nombre de la suite actual
    std::string current_suite_name;
    
    /// Timeout del test actual (ms)
    uint32_t test_timeout_ms = 30000;
    
    /// Número de retries permitidos
    int max_retries = 0;
    
    // ─────────────────────────────────────────────────────────────────────
    // BANDERAS DE ESTADO
    // ─────────────────────────────────────────────────────────────────────
    
    /// Indica si el test está corriendo
    bool is_running = false;
    
    /// Indica si el test debe saltarse
    bool should_skip = false;
    
    /// Razón del skip
    std::string skip_reason;
    
    // ─────────────────────────────────────────────────────────────────────
    // MÉTODOS
    // ─────────────────────────────────────────────────────────────────────
    
    TestContext() {
        // Por defecto, política DEV
        security_policy = std::make_unique<DevSecurityPolicy>();
    }
    
    /// Inicializar contexto con configuración
    void initialize(const BackendConfig& config, SecurityPolicy policy) {
        backend_config = config;

        // Configurar reloj según backend
        clock_manager.set_simulation_mode(config.backend_type == "sim");
        
        // Configurar política
        switch (policy) {
            case SecurityPolicy::DEV:
                security_policy = std::make_unique<DevSecurityPolicy>();
                break;
            case SecurityPolicy::PROD:
                security_policy = std::make_unique<ProdSecurityPolicy>();
                break;
        }
        
        // Recolectar información del sistema
        collect_system_info();
    }

    /// Adjuntar backend (no ownership)
    void attach_backend(ICanBackend* backend_ptr) {
        backend = backend_ptr;
    }
    
    /// Verificar si el test puede ejecutarse con el backend actual
    bool check_requirements(const TestMetadata& meta, std::string& reason) const {
        if (should_skip) {
            reason = skip_reason.empty() ? "Global skip flag" : skip_reason;
            return false;
        }
        
        if (meta.required_backend == RequiredBackend::SIM_ONLY && 
            backend_config.backend_type != "sim") {
            reason = "Requires SIM backend";
            return false;
        }
        
        if (meta.required_backend == RequiredBackend::REAL_ONLY && 
            backend_config.backend_type == "sim") {
            reason = "Requires REAL backend";
            return false;
        }
        
        if (meta.requires_sched_fifo && !system_info.sched_fifo_available) {
            reason = "SCHED_FIFO not available (CAP_SYS_NICE missing)";
            return false;
        }
        
        if (meta.requires_can && !system_info.can_interface_exists) {
            reason = "No CAN interface detected";
            return false;
        }
        
        if (meta.requires_hmi_socket && !system_info.hmi_socket_exists) {
            reason = "HMI socket not available";
            return false;
        }
        
        return true;
    }
    
    /// Obtener tiempo actual
    uint64_t now_ms() const {
        return clock_manager.now_ms();
    }
    
    /// Obtener tiempo en microsegundos
    uint64_t now_us() const {
        return clock_manager.now_us();
    }
    
private:
    void collect_system_info() {
        system_info.cpu_count = std::thread::hardware_concurrency();
        system_info.test_start_time_ms = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

#ifdef __linux__
        // Kernel / Arch
        struct utsname uname_data;
        if (uname(&uname_data) == 0) {
            system_info.kernel_version = uname_data.release;
            system_info.architecture = uname_data.machine;
        }

        // PREEMPT_RT detection
        {
            std::ifstream rt_file("/sys/kernel/realtime");
            std::string rt_val;
            if (std::getline(rt_file, rt_val)) {
                system_info.preempt_rt_detected = (rt_val.find("1") != std::string::npos);
            }
            if (!system_info.preempt_rt_detected) {
                std::ifstream cmdline("/proc/cmdline");
                std::string line;
                if (std::getline(cmdline, line)) {
                    if (line.find("PREEMPT_RT") != std::string::npos ||
                        line.find("preempt_rt") != std::string::npos) {
                        system_info.preempt_rt_detected = true;
                    }
                }
            }
        }

        // CPU model
        {
            std::ifstream cpuinfo("/proc/cpuinfo");
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.find("model name") != std::string::npos) {
                    auto pos = line.find(':');
                    if (pos != std::string::npos) {
                        system_info.cpu_model = line.substr(pos + 1);
                        // trim leading spaces
                        while (!system_info.cpu_model.empty() && system_info.cpu_model[0] == ' ') {
                            system_info.cpu_model.erase(0, 1);
                        }
                    }
                    break;
                }
            }
        }

        // Capabilities / root
        system_info.has_root = (geteuid() == 0);
        system_info.has_cap_sys_nice = false;
        {
            std::ifstream status("/proc/self/status");
            std::string line;
            while (std::getline(status, line)) {
                if (line.rfind("CapEff:", 0) == 0) {
                    std::string hex = line.substr(7);
                    while (!hex.empty() && hex[0] == ' ') hex.erase(0, 1);
                    unsigned long long cap_eff = 0;
                    try {
                        cap_eff = std::stoull(hex, nullptr, 16);
                    } catch (...) {
                        cap_eff = 0;
                    }
                    // CAP_SYS_NICE = 23
                    system_info.has_cap_sys_nice = (cap_eff & (1ULL << 23)) != 0;
                    break;
                }
            }
        }
        system_info.sched_fifo_available = system_info.has_root || system_info.has_cap_sys_nice;

        // Boot time (from /proc/stat btime)
        {
            std::ifstream stat("/proc/stat");
            std::string line;
            while (std::getline(stat, line)) {
                if (line.rfind("btime ", 0) == 0) {
                    try {
                        uint64_t btime_s = std::stoull(line.substr(6));
                        system_info.boot_time_ms = btime_s * 1000ULL;
                    } catch (...) {
                        system_info.boot_time_ms = 0;
                    }
                    break;
                }
            }
        }

        // CAN interfaces
        system_info.can_interface_exists = false;
        system_info.can_interface_name.clear();
        DIR* dir = opendir("/sys/class/net");
        if (dir) {
            struct dirent* ent;
            while ((ent = readdir(dir)) != nullptr) {
                std::string ifname = ent->d_name;
                if (ifname.rfind("can", 0) == 0 || ifname.rfind("emucc", 0) == 0) {
                    system_info.can_interface_exists = true;
                    if (system_info.can_interface_name.empty()) {
                        system_info.can_interface_name = ifname;
                    }
                }
            }
            closedir(dir);
        }

        // HMI socket
        system_info.hmi_socket_exists = (access("/run/ecu/errors.sock", F_OK) == 0);
#else
        // Fallback for non-Linux builds
        system_info.kernel_version = "unknown";
        system_info.architecture = "unknown";
        system_info.cpu_model = "unknown";
        system_info.preempt_rt_detected = false;
        system_info.sched_fifo_available = false;
        system_info.can_interface_exists = false;
        system_info.hmi_socket_exists = false;
#endif
    }
};

// ============================================================================
// SECURITY POLICY FACTORY
// ============================================================================

inline std::unique_ptr<ISecurityPolicy> create_security_policy(SecurityPolicy type) {
    switch (type) {
        case SecurityPolicy::DEV:
            return std::make_unique<DevSecurityPolicy>();
        case SecurityPolicy::PROD:
            return std::make_unique<ProdSecurityPolicy>();
        default:
            return std::make_unique<DevSecurityPolicy>();
    }
}

} // namespace testing
} // namespace ecu
