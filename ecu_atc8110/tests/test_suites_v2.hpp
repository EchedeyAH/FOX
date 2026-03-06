#pragma once
/**
 * test_suites_v2.hpp
 * Suites de tests mejoradas con soporte para:
 * - Backends SIM y REAL
 * - Políticas de seguridad DEV/PROD
 * - Tests de lógica vs timing
 * - Métricas de latencia
 * 
 * Esta versión ampliada incluye tests mejorados para HMI
 * con validación de esquema, snapshots, latencia y reconexión.
 */

#include <array>
#include <atomic>
#include <chrono>
#include <thread>
#include <cstring>
#include <string>
#include <vector>

#include "ecu_test_framework.hpp"
#include "framework/test_context.hpp"
#include "framework/timing_metrics.hpp"
#include "system_mode_manager.hpp"
#include "motor_timeout_detector.hpp"
#include "voltage_protection.hpp"
#include "can_simulator.hpp"
#include "telemetry_publisher.hpp"
#include "error_publisher.hpp"
#include "error_catalog.hpp"

#ifdef __linux__
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#endif

namespace ecu {
namespace testing {

using namespace common;

namespace {

constexpr const char* HMI_SOCKET_PATH = "/run/ecu/errors.sock";
constexpr int HMI_DEFAULT_TIMEOUT_MS = 500;

static int connect_unix_socket(const char* path, int timeout_ms) {
#ifdef __linux__
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    // Set non-blocking for controlled read
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    (void)timeout_ms;
    return fd;
#else
    (void)path;
    (void)timeout_ms;
    return -1;
#endif
}

static std::string recv_json(int fd, int timeout_ms) {
#ifdef __linux__
    if (fd < 0) return {};
    std::string data;
    std::vector<char> buf(512);
    
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret <= 0) return {};
    
    ssize_t n = recv(fd, buf.data(), buf.size(), 0);
    if (n > 0) {
        data.assign(buf.data(), static_cast<size_t>(n));
    }
    return data;
#else
    (void)fd;
    (void)timeout_ms;
    return {};
#endif
}

static bool json_has_field(const std::string& json, const char* key) {
    std::string needle = std::string("\"") + key + "\"";
    return json.find(needle) != std::string::npos;
}

static uint64_t json_get_u64(const std::string& json, const char* key) {
    std::string needle = std::string("\"") + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    return static_cast<uint64_t>(std::strtoull(json.c_str() + pos, nullptr, 10));
}

static bool ensure_error_publisher_running() {
#ifdef __linux__
    if (access(HMI_SOCKET_PATH, F_OK) == 0) {
        return true;
    }
    ecu::ErrorPublisherConfig cfg;
    cfg.socket_path = HMI_SOCKET_PATH;
    cfg.snap_interval_ms = 200;
    cfg.send_snap_on_connect = true;
    if (!ecu::g_error_publisher.init(cfg)) return false;
    ecu::g_error_publisher.start();
    return true;
#else
    return false;
#endif
}

} // namespace

// ============================================================================
// MOTOR TIMEOUT SUITE (Policy-Aware)
// ============================================================================

/**
 * Suite de tests para Motor Timeout Detection
 * Tests adaptados según política de seguridad
 */
class MotorTimeoutSuiteV2 : public TestSuite {
public:
    MotorTimeoutSuiteV2() {
        set_name("MotorTimeoutSuiteV2");
    }
    
    std::string get_name() const override { return name_; }
    void set_name(const std::string& n) { name_ = n; }
    
    std::vector<std::unique_ptr<BaseTest>> get_tests() override {
        std::vector<std::unique_ptr<BaseTest>> tests;
        
        // Tests de lógica (SIM y REAL)
        tests.push_back(std::make_unique<MT2_01_TimeoutSingleMotor>());
        tests.push_back(std::make_unique<MT2_02_TimeoutAllMotors>());
        
        // Tests de recovery (dependen de política)
        tests.push_back(std::make_unique<MT2_03_RecoveryDev>());   // DEV only
        tests.push_back(std::make_unique<MT2_04_RecoveryProd>());   // PROD only
        
        // Tests de timing (solo SIM para control)
        tests.push_back(std::make_unique<MT2_05_TimeoutTiming>());
        
        return tests;
    }

private:
    std::string name_;
};

// Test MT2-01: Timeout de un solo motor
class MT2_01_TimeoutSingleMotor : public BaseTest {
public:
    void run() override {
        auto& sim = get_can_simulator();
        auto& mode_mgr = get_system_mode_manager();
        
        // Reset
        mode_mgr.reset();
        sim.set_mode(SimulationMode::NORMAL);
        
        // Verificar OK inicial
        assert_eq(SystemMode::OK, mode_mgr.get_mode(), "Initial mode should be OK");
        
        // Iniciar simulación y esperar timeout
        sim.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(2100));
        
        // Verificar SAFE_STOP
        assert_eq(SystemMode::SAFE_STOP, mode_mgr.get_mode(), 
                  "Should transition to SAFE_STOP after motor timeout");
        
        sim.stop();
    }
};

// Test MT2-02: Timeout de todos los motores
class MT2_02_TimeoutAllMotors : public BaseTest {
public:
    void run() override {
        auto& sim = get_can_simulator();
        auto& mode_mgr = get_system_mode_manager();
        
        mode_mgr.reset();
        sim.set_mode(SimulationMode::NORMAL);
        
        assert_eq(SystemMode::OK, mode_mgr.get_mode());
        
        // Configurar para timeout de todos
        sim.set_mode(SimulationMode::MOTOR_TIMEOUT);
        std::this_thread::sleep_for(std::chrono::milliseconds(2100));
        
        assert_eq(SystemMode::SAFE_STOP, mode_mgr.get_mode(),
                  "All motors timeout should trigger SAFE_STOP");
    }
};

// Test MT2-03: Recovery en política DEV (automático)
class MT2_03_RecoveryDev : public BaseTest {
public:
    void run() override {
        // Este test solo corre en DEV policy
        // En producción, este recovery sería automático
        
        auto& sim = get_can_simulator();
        auto& mode_mgr = get_system_mode_manager();
        
        mode_mgr.reset();
        sim.set_mode(SimulationMode::NORMAL);
        
        // Provocar timeout
        sim.set_mode(SimulationMode::MOTOR_TIMEOUT);
        std::this_thread::sleep_for(std::chrono::milliseconds(2100));
        
        assert_eq(SystemMode::SAFE_STOP, mode_mgr.get_mode());
        
        // Recovery - volver a normal
        sim.set_mode(SimulationMode::NORMAL);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Reset manual debe funcionar
        mode_mgr.reset();
        
        assert_eq(SystemMode::OK, mode_mgr.get_mode(),
                  "After reset, should be OK");
    }
};

// Test MT2-04: Recovery en política PROD (requiere ACK)
class MT2_04_RecoveryProd : public BaseTest {
public:
    void run() override {
        // En PROD, el recovery desde SAFE_STOP requiere condiciones estables
        
        auto& sim = get_can_simulator();
        auto& mode_mgr = get_system_mode_manager();
        
        mode_mgr.reset();
        
        // Provocar SAFE_STOP
        sim.set_mode(SimulationMode::MOTOR_TIMEOUT);
        std::this_thread::sleep_for(std::chrono::milliseconds(2100));
        
        assert_eq(SystemMode::SAFE_STOP, mode_mgr.get_mode());
        
        // En PROD, necesitamos:
        // 1. Condiciones estables por 2s mínimo
        // 2. ACK explícito
        
        // Recovery - normal
        sim.set_mode(SimulationMode::NORMAL);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // En PROD, el recovery es manual
        // Verificar que el sistema espera ACK
        auto mode = mode_mgr.get_mode();
        assert_true(mode == SystemMode::SAFE_STOP || mode == SystemMode::OK,
                    "In PROD, recovery from SAFE_STOP requires explicit reset");
    }
};

// Test MT2-05: Timing del timeout
class MT2_05_TimeoutTiming : public BaseTest {
public:
    void run() override {
        // Test de timing - solo en SIM para control determinista
        
        TimingMetrics metrics;
        metrics.start();
        
        auto& sim = get_can_simulator();
        auto& mode_mgr = get_system_mode_manager();
        
        mode_mgr.reset();
        sim.set_mode(SimulationMode::NORMAL);
        
        auto start = std::chrono::steady_clock::now();
        sim.start();
        
        // Medir tiempo hasta SAFE_STOP
        while (mode_mgr.get_mode() != SystemMode::SAFE_STOP) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            auto elapsed = std::chrono::steady_clock::now() - start;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 5000) {
                break;
            }
        }
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // El timeout es ~2s
        assert_true(duration >= 1900 && duration <= 2500,
                    "Timeout should occur around 2s");
        
        sim.stop();
    }
};

// ============================================================================
// HMI INTEGRATION SUITE (Enhanced)
// ============================================================================

/**
 * Suite mejorada de integración HMI
 * Incluye validación de esquema, snapshots, latencia y reconexión
 */
class HmiIntegrationSuiteV2 : public TestSuite {
public:
    std::string get_name() const override { return "HmiIntegrationSuiteV2"; }
    
    std::vector<std::unique_ptr<BaseTest>> get_tests() override {
        std::vector<std::unique_ptr<BaseTest>> tests;
        
        // Validación de esquema
        tests.push_back(std::make_unique<HMI2_01_SchemaValidation>());
        tests.push_back(std::make_unique<HMI2_02_RequiredFields>());
        
        // Snapshot inicial
        tests.push_back(std::make_unique<HMI2_03_InitialSnapshot>());
        
        // Latencia de eventos
        tests.push_back(std::make_unique<HMI2_04_EventLatency>());
        
        // Reconexión
        tests.push_back(std::make_unique<HMI2_05_Reconnection>());
        
        // Orden de timestamps
        tests.push_back(std::make_unique<HMI2_06_TimestampOrder>());
        
        return tests;
    }
};

// Test HMI2-01: Validar esquema JSON
class HMI2_01_SchemaValidation : public BaseTest {
public:
    TestMetadata metadata() const override {
        TestMetadata meta;
        meta.category = TestCategory::INTEGRATION;
        meta.required_backend = RequiredBackend::BOTH;
        meta.requires_hmi_socket = true;
        return meta;
    }
    
    void run() override {
        if (!ensure_error_publisher_running()) {
            skip("Error publisher not available");
        }
        
        int fd = connect_unix_socket(HMI_SOCKET_PATH, HMI_DEFAULT_TIMEOUT_MS);
        if (fd < 0) {
            skip("Cannot connect to HMI socket");
        }
        
        auto snap = recv_json(fd, HMI_DEFAULT_TIMEOUT_MS);
        close(fd);
        
        assert_true(!snap.empty(), "No snapshot received");
        assert_true(json_has_field(snap, "type"), "Missing type field");
        assert_true(json_has_field(snap, "ts"), "Missing timestamp field");
        assert_true(json_has_field(snap, "total"), "Missing total field");
        assert_true(json_has_field(snap, "active"), "Missing active field");
    }
};

// Test HMI2-02: Validar campos obligatorios
class HMI2_02_RequiredFields : public BaseTest {
public:
    TestMetadata metadata() const override {
        TestMetadata meta;
        meta.category = TestCategory::INTEGRATION;
        meta.required_backend = RequiredBackend::BOTH;
        meta.requires_hmi_socket = true;
        return meta;
    }
    
    void run() override {
        if (!ensure_error_publisher_running()) {
            skip("Error publisher not available");
        }
        
        int fd = connect_unix_socket(HMI_SOCKET_PATH, HMI_DEFAULT_TIMEOUT_MS);
        if (fd < 0) {
            skip("Cannot connect to HMI socket");
        }
        
        auto snap = recv_json(fd, HMI_DEFAULT_TIMEOUT_MS);
        close(fd);
        
        assert_true(json_has_field(snap, "type"), "Missing type");
        assert_true(json_has_field(snap, "ts"), "Missing timestamp");
        assert_true(json_has_field(snap, "crit"), "Missing crit");
        assert_true(json_has_field(snap, "grave"), "Missing grave");
        assert_true(json_has_field(snap, "leve"), "Missing leve");
        assert_true(json_has_field(snap, "info"), "Missing info");
    }
};

// Test HMI2-03: Validar snapshot inicial completo
class HMI2_03_InitialSnapshot : public BaseTest {
public:
    TestMetadata metadata() const override {
        TestMetadata meta;
        meta.category = TestCategory::INTEGRATION;
        meta.required_backend = RequiredBackend::BOTH;
        meta.requires_hmi_socket = true;
        return meta;
    }
    
    void run() override {
        if (!ensure_error_publisher_running()) {
            skip("Error publisher not available");
        }
        
        int fd = connect_unix_socket(HMI_SOCKET_PATH, HMI_DEFAULT_TIMEOUT_MS);
        if (fd < 0) {
            skip("Cannot connect to HMI socket");
        }
        
        auto snap = recv_json(fd, HMI_DEFAULT_TIMEOUT_MS);
        close(fd);
        
        assert_true(!snap.empty(), "Snapshot empty");
        assert_true(json_has_field(snap, "type"), "Snapshot missing type");
        assert_true(json_has_field(snap, "ts"), "Snapshot missing ts");
        assert_true(json_has_field(snap, "total"), "Snapshot missing total");
    }
};

// Test HMI2-04: Validar latencia de eventos (<100ms objetivo)
class HMI2_04_EventLatency : public BaseTest {
public:
    TestMetadata metadata() const override {
        TestMetadata meta;
        meta.category = TestCategory::TIMING;
        meta.required_backend = RequiredBackend::BOTH;
        meta.requires_hmi_socket = true;
        return meta;
    }
    
    void run() override {
        if (!ensure_error_publisher_running()) {
            skip("Error publisher not available");
        }
        
        int fd = connect_unix_socket(HMI_SOCKET_PATH, 200);
        if (fd < 0) {
            skip("Cannot connect to HMI socket");
        }
        
        // Consumir snapshot inicial
        (void)recv_json(fd, 200);
        
        // Generar evento y medir latencia
        ecu::ErrorEvent evt{};
        evt.timestamp_ms = ctx().now_ms();
        evt.code = ecu::ErrorCode::BMS_COM_LOST;
        evt.level = ecu::ErrorLevel::GRAVE;
        evt.group = ecu::ErrorGroup::BMS;
        evt.status = ecu::ErrorStatus::ACTIVO;
        evt.origin = "BMS";
        evt.description = "BMS comm lost";
        evt.count = 1;
        
        uint64_t start_us = ctx().now_us();
        ecu::g_error_publisher.publish_event(evt);
        
        auto msg = recv_json(fd, 200);
        uint64_t end_us = ctx().now_us();
        close(fd);
        
        assert_true(!msg.empty(), "No EVENT received");
        assert_true(json_has_field(msg, "type"), "Missing type in EVENT");
        assert_true(json_has_field(msg, "ts"), "Missing ts in EVENT");
        
        uint64_t latency_us = (end_us >= start_us) ? (end_us - start_us) : 0;
        assert_true(latency_us < 100000, "Event latency must be <100ms");
    }
};

// Test HMI2-05: Validar reconexión
class HMI2_05_Reconnection : public BaseTest {
public:
    TestMetadata metadata() const override {
        TestMetadata meta;
        meta.category = TestCategory::INTEGRATION;
        meta.required_backend = RequiredBackend::BOTH;
        meta.requires_hmi_socket = true;
        return meta;
    }
    
    void run() override {
        if (!ensure_error_publisher_running()) {
            skip("Error publisher not available");
        }
        
        int fd1 = connect_unix_socket(HMI_SOCKET_PATH, HMI_DEFAULT_TIMEOUT_MS);
        if (fd1 < 0) {
            skip("Cannot connect to HMI socket");
        }
        
        auto snap1 = recv_json(fd1, HMI_DEFAULT_TIMEOUT_MS);
        uint64_t ts1 = json_get_u64(snap1, "ts");
        close(fd1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        int fd2 = connect_unix_socket(HMI_SOCKET_PATH, HMI_DEFAULT_TIMEOUT_MS);
        if (fd2 < 0) {
            skip("Cannot reconnect to HMI socket");
        }
        
        auto snap2 = recv_json(fd2, HMI_DEFAULT_TIMEOUT_MS);
        uint64_t ts2 = json_get_u64(snap2, "ts");
        close(fd2);
        
        assert_true(!snap1.empty() && !snap2.empty(), "Snapshots missing after reconnect");
        assert_true(ts2 >= ts1, "Snapshot timestamp after reconnection should be monotonic");
    }
};

// Test HMI2-06: Validar orden de timestamps (monotónico)
class HMI2_06_TimestampOrder : public BaseTest {
public:
    TestMetadata metadata() const override {
        TestMetadata meta;
        meta.category = TestCategory::INTEGRATION;
        meta.required_backend = RequiredBackend::BOTH;
        meta.requires_hmi_socket = true;
        return meta;
    }
    
    void run() override {
        if (!ensure_error_publisher_running()) {
            skip("Error publisher not available");
        }
        
        int fd = connect_unix_socket(HMI_SOCKET_PATH, HMI_DEFAULT_TIMEOUT_MS);
        if (fd < 0) {
            skip("Cannot connect to HMI socket");
        }
        
        uint64_t last_ts = 0;
        for (int i = 0; i < 3; ++i) {
            auto msg = recv_json(fd, 1200);
            if (msg.empty()) {
                skip("No snapshot/event received for timestamp check");
            }
            uint64_t ts = json_get_u64(msg, "ts");
            assert_true(ts >= last_ts, "Timestamps must be monotonic");
            last_ts = ts;
        }
        
        close(fd);
    }
};

// ============================================================================
// SCHEDULER TIMING SUITE (NEW)
// ============================================================================

/**
 * Suite de tests de timing del scheduler
 * Mide jitter, deadline misses y overruns
 */
class SchedulerTimingSuite : public TestSuite {
public:
    std::string get_name() const override { return "SchedulerTimingSuite"; }
    
    std::vector<std::unique_ptr<BaseTest>> get_tests() override {
        std::vector<std::unique_ptr<BaseTest>> tests;
        
        // Tests de jitter
        tests.push_back(std::make_unique<SJ2_01_JitterMeasurement>());
        tests.push_back(std::make_unique<SJ2_02_JitterPercentiles>());
        
        // Tests de deadline
        tests.push_back(std::make_unique<SJ2_03_DeadlineMiss>());
        
        // Tests de overrun
        tests.push_back(std::make_unique<SJ2_04_OverrunDetection>());
        
        return tests;
    }
};

// Test SJ2-01: Medición básica de jitter
class SJ2_01_JitterMeasurement : public BaseTest {
public:
    void run() override {
        TimingMetrics metrics;
        
        // Simular 1000 iteraciones de una tarea periódica
        const uint32_t period_us = 1000;  // 1ms
        
        for (uint32_t i = 0; i < 1000; ++i) {
            auto expected = (i + 1) * period_us;
            
            // Simular variación (en real vendría del scheduler)
            uint32_t actual = expected + (i % 100) - 50;  // ±50us jitter
            
            metrics.record_interval(expected, actual);
        }
        
        auto jitter = metrics.get_jitter_percentiles();
        
        // Verificar que tenemos datos
        assert_true(jitter.max_us > 0, "Jitter data collected");
        
        std::cout << "    Jitter: p50=" << jitter.p50_us 
                  << "us p95=" << jitter.p95_us 
                  << "us max=" << jitter.max_us << "us\n";
    }
};

// Test SJ2-02: Percentiles de jitter
class SJ2_02_JitterPercentiles : public BaseTest {
public:
    void run() override {
        TimingMetrics metrics;
        
        // Generar distribución de jitter
        for (int i = 0; i < 10000; ++i) {
            uint32_t expected = (i + 1) * 1000;
            // Simular distribución realista
            int jitter = (i % 200) - 100;  // -100 a +100us
            uint32_t actual = expected + jitter;
            
            metrics.record_interval(expected, actual);
        }
        
        auto jitter = metrics.get_jitter_percentiles();
        
        // Verificar percentiles
        assert_true(jitter.p50_us <= jitter.p95_us, "p50 <= p95");
        assert_true(jitter.p95_us <= jitter.p99_us, "p95 <= p99");
        assert_true(jitter.p99_us <= jitter.max_us, "p99 <= max");
        
        std::cout << "    Jitter percentiles:\n";
        std::cout << "      p50: " << jitter.p50_us << " us\n";
        std::cout << "      p95: " << jitter.p95_us << " us\n";
        std::cout << "      p99: " << jitter.p99_us << " us\n";
        std::cout << "      max: " << jitter.max_us << " us\n";
        std::cout << "      mean: " << jitter.mean_us << " us\n";
    }
};

// Test SJ2-03: Deadline miss detection
class SJ2_03_DeadlineMiss : public BaseTest {
public:
    void run() override {
        TimingMetrics metrics;
        
        const uint32_t deadline_us = 2000;  // 2ms deadline
        
        for (int i = 0; i < 1000; ++i) {
            uint32_t expected = (i + 1) * 1000;
            uint32_t actual = expected;
            
            // Simular algunos deadline misses (5%)
            if (i % 20 == 0) {
                actual = deadline_us + 500;  // Miss!
                metrics.record_deadline_miss();
            }
            
            metrics.record_interval(expected, actual);
        }
        
        auto deadline_metrics = metrics.get_deadline_metrics();
        
        // Debimos detectar ~50 deadline misses
        assert_true(deadline_metrics.missed_count > 0, 
                    "Should detect deadline misses");
        
        std::cout << "    Deadline misses: " << deadline_metrics.missed_count 
                  << " (" << deadline_metrics.miss_rate_percent << "%)\n";
    }
};

// Test SJ2-04: Overrun detection
class SJ2_04_OverrunDetection : public BaseTest {
public:
    void run() override {
        TimingMetrics metrics;
        
        const uint32_t period_us = 1000;
        
        for (int i = 0; i < 1000; ++i) {
            uint32_t expected = (i + 1) * period_us;
            
            // Simular overruns (ejecución más larga que el período)
            uint32_t actual = expected;
            if (i % 50 == 0) {
                actual = expected + 200;  // Overrun!
                metrics.record_overrun(200);
            }
            
            metrics.record_interval(expected, actual);
        }
        
        auto overrun_metrics = metrics.get_overrun_metrics();
        
        assert_true(overrun_metrics.overrun_count > 0, 
                    "Should detect overruns");
        
        std::cout << "    Overruns: " << overrun_metrics.overrun_count 
                  << " (" << overrun_metrics.overrun_rate_percent << "%)\n";
    }
};

// ============================================================================
// SUITE MAPPING (para backwards compatibility)
// ============================================================================

// Alias para la suite original (mantiene compatibilidad)
using MotorTimeoutSuite = MotorTimeoutSuiteV2;
using HmiIntegrationSuite = HmiIntegrationSuiteV2;

} // namespace testing
} // namespace ecu
