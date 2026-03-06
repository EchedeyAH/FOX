#pragma once
/**
 * ecu_test_framework.hpp
 * Framework de pruebas para sistema ECU ATC8110
 * 
 * Proporciona:
 * - Ejecución automática de tests
 * - Registro PASS/FAIL
 * - Medición de tiempos
 * - Reportes JSON y Markdown
 * - Integración con CanSimulator
 * 
 * Uso:
 *   ECUTestFramework framework;
 *   framework.add_suite<MotorTimeoutSuite>();
 *   framework.add_suite<VoltageSuite>();
 *   framework.run();
 *   framework.generate_report();
 */

#include <atomic>
#include <array>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "system_mode_manager.hpp"
#include "motor_timeout_detector.hpp"
#include "voltage_protection.hpp"
#include "can_simulator.hpp"
#include "telemetry_publisher.hpp"
#include "framework/test_context.hpp"

namespace ecu {
namespace testing {

// ============================================================================
// CONSTANTES
// ============================================================================

constexpr const char* TEST_REPORT_JSON = "test_report.json";
constexpr const char* TEST_REPORT_MD = "test_report.md";
constexpr int MAX_TEST_NAME_LEN = 64;

// ============================================================================
// ENUMS
// ============================================================================

enum class TestResult {
    NOT_RUN = 0,
    PASS = 1,
    FAIL = 2,
    SKIPPED = 3,
    TIMEOUT = 4
};

enum class TestSeverity {
    LOW = 0,
    MEDIUM = 1,
    HIGH = 2,
    CRITICAL = 3
};

// ============================================================================
// ESTRUCTURAS
// ============================================================================

/**
 * Resultado de un test individual
 */
struct TestResultData {
    std::string name;
    std::string suite;
    TestResult result = TestResult::NOT_RUN;
    TestSeverity severity = TestSeverity::MEDIUM;
    std::string message;
    std::string expected;
    std::string actual;
    
    // Tiempos
    uint64_t duration_us = 0;
    uint64_t setup_time_us = 0;
    uint64_t teardown_time_us = 0;
    
    // Timestamps
    uint64_t start_timestamp_ms = 0;
    uint64_t end_timestamp_ms = 0;
    
    // Metadata
    std::string file;
    int line = 0;
};

/**
 * Métricas del sistema durante un test
 */
struct SystemMetrics {
    double vbat_mv = 0;
    double soc_percent = 0;
    double temp_battery_c = 0;
    std::array<double, 4> temp_motors_c = {0, 0, 0, 0};
    common::SystemMode mode = common::SystemMode::OK;
    double power_factor = 1.0;
    bool torque_zero = false;
    int timeout_count = 0;
    int64_t jitter_us = 0;
};

// ============================================================================
// TEST ASSERTIONS
// ============================================================================

class AssertionError : public std::exception {
public:
    AssertionError(const std::string& msg) : message(msg) {}
    const char* what() const noexcept { return message.c_str(); }
private:
    std::string message;
};

class SkipTest : public std::exception {
public:
    SkipTest(const std::string& msg) : message(msg) {}
    const char* what() const noexcept { return message.c_str(); }
private:
    std::string message;
};

inline void assert_true(bool condition, const std::string& msg = "") {
    if (!condition) {
        throw AssertionError(msg.empty() ? "Assertion failed: expected true" : msg);
    }
}

inline void assert_false(bool condition, const std::string& msg = "") {
    if (condition) {
        throw AssertionError(msg.empty() ? "Assertion failed: expected false" : msg);
    }
}

inline void assert_eq(int expected, int actual, const std::string& msg = "") {
    if (expected != actual) {
        std::ostringstream ss;
        ss << (msg.empty() ? "Assertion failed" : msg) 
           << " - expected: " << expected << ", actual: " << actual;
        throw AssertionError(ss.str());
    }
}

inline void assert_eq(double expected, double actual, double epsilon = 0.001, 
                      const std::string& msg = "") {
    if (std::abs(expected - actual) > epsilon) {
        std::ostringstream ss;
        ss << (msg.empty() ? "Assertion failed" : msg) 
           << " - expected: " << expected << ", actual: " << actual;
        throw AssertionError(ss.str());
    }
}

inline void assert_eq(common::SystemMode expected, common::SystemMode actual,
                      const std::string& msg = "") {
    if (expected != actual) {
        std::ostringstream ss;
        ss << (msg.empty() ? "Assertion failed" : msg) 
           << " - expected: " << static_cast<int>(expected) 
           << ", actual: " << static_cast<int>(actual);
        throw AssertionError(ss.str());
    }
}

inline void assert_ge(int expected, int actual, const std::string& msg = "") {
    if (actual < expected) {
        std::ostringstream ss;
        ss << (msg.empty() ? "Assertion failed" : msg) 
           << " - expected: >=" << expected << ", actual: " << actual;
        throw AssertionError(ss.str());
    }
}

inline void assert_le(int expected, int actual, const std::string& msg = "") {
    if (actual > expected) {
        std::ostringstream ss;
        ss << (msg.empty() ? "Assertion failed" : msg) 
           << " - expected: <=" << expected << ", actual: " << actual;
        throw AssertionError(ss.str());
    }
}

// ============================================================================
// TEST BASE
// ============================================================================

/**
 * Clase base para todos los tests
 */
class BaseTest {
public:
    virtual ~BaseTest() = default;
    
    virtual void setup() {}
    virtual void run() = 0;
    virtual void teardown() {}
    virtual TestMetadata metadata() const { return {}; }
    
    const std::string& get_name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }
    
    TestResult get_result() const { return result_; }
    const std::string& get_message() const { return message_; }
    uint64_t get_duration_us() const { return duration_us_; }
    
    void set_context(TestContext* ctx) { ctx_ = ctx; }
    TestContext& ctx() { return *ctx_; }
    const TestContext& ctx() const { return *ctx_; }
    
protected:
    void skip(const std::string& reason) {
        throw SkipTest(reason);
    }
    
protected:
    TestContext* ctx_ = nullptr;
    std::string name_;
    TestResult result_ = TestResult::NOT_RUN;
    std::string message_;
    uint64_t duration_us_ = 0;
};

// ============================================================================
// TEST SUITE INTERFACE
// ============================================================================

/**
 * Interfaz para suites de tests
 */
class TestSuite {
public:
    virtual ~TestSuite() = default;
    
    virtual void setup() {}
    virtual void teardown() {}
    virtual std::vector<std::unique_ptr<BaseTest>> get_tests() = 0;
    virtual std::string get_name() const = 0;
    
    int get_pass_count() const { return pass_count_; }
    int get_fail_count() const { return fail_count_; }
    int get_skip_count() const { return skip_count_; }
    
protected:
    int pass_count_ = 0;
    int fail_count_ = 0;
    int skip_count_ = 0;
};

// ============================================================================
// ECU TEST FRAMEWORK
// ============================================================================

/**
 * Framework principal de pruebas
 */
class ECUTestFramework {
public:
    ECUTestFramework();
    ~ECUTestFramework();
    
    // ─────────────────────────────────────────────────────────────────────
    // CONFIGURACIÓN
    // ─────────────────────────────────────────────────────────────────────
    
    // Añadir suite de tests
    template<typename T>
    void add_suite() {
        static_assert(std::is_base_of<TestSuite, T>::value, 
                      "T must derive from TestSuite");
        auto suite = std::make_unique<T>();
        suite->setup();
        suites_.push_back(std::move(suite));
    }
    
    // Configurar simulator
    void set_simulator(common::CanSimulator* sim) { simulator_ = sim; }
    
    // Configurar mode manager
    void set_mode_manager(common::SystemModeManager* mgr) { mode_manager_ = mgr; }
    
    // Configurar contexto de test
    void set_test_context(TestContext* ctx) { context_ = ctx; }
    
    // ─────────────────────────────────────────────────────────────────────
    // EJECUCIÓN
    // ─────────────────────────────────────────────────────────────────────
    
    // Ejecutar todos los tests
    void run();
    
    // Ejecutar suite específica
    void run_suite(const std::string& suite_name);
    
    // Ejecutar test específico
    void run_test(const std::string& test_name);
    
    // ─────────────────────────────────────────────────────────────────────
    // REPORTES
    // ─────────────────────────────────────────────────────────────────────
    
    // Generar reporte JSON
    void generate_json_report(const std::string& filename = TEST_REPORT_JSON);
    
    // Generar reporte Markdown
    void generate_markdown_report(const std::string& filename = TEST_REPORT_MD);
    
    // Generar ambos reportes
    void generate_report() {
        generate_json_report();
        generate_markdown_report();
    }
    
    // ─────────────────────────────────────────────────────────────────────
    // CONSULTA
    // ─────────────────────────────────────────────────────────────────────
    
    int get_total_tests() const { return total_tests_; }
    int get_pass_count() const { return pass_count_; }
    int get_fail_count() const { return fail_count_; }
    int get_skip_count() const { return skip_count_; }
    
    const std::vector<TestResultData>& get_results() const { return results_; }
    
    // Print resumen a consola
    void print_summary();

private:
    // Suites
    std::vector<std::unique_ptr<TestSuite>> suites_;
    
    // Referencias a módulos del sistema
    common::CanSimulator* simulator_ = nullptr;
    common::SystemModeManager* mode_manager_ = nullptr;
    TestContext* context_ = nullptr;
    
    // Resultados
    std::vector<TestResultData> results_;
    int total_tests_ = 0;
    int pass_count_ = 0;
    int fail_count_ = 0;
    int skip_count_ = 0;
    
    // Métricas globales
    uint64_t total_duration_us_ = 0;
    uint64_t framework_start_ms_ = 0;
    uint64_t framework_end_ms_ = 0;
    
    // Control
    std::atomic<bool> running_;
    std::mutex mutex_;
    
    // Helpers
    void run_single_test(BaseTest* test, const std::string& suite_name);
    void capture_system_metrics(SystemMetrics& metrics);
    std::string get_result_string(TestResult result);
};

// ============================================================================
// IMPLEMENTACIÓN
// ============================================================================

inline ECUTestFramework::ECUTestFramework() : running_(false) {
    results_.reserve(100);
}

inline ECUTestFramework::~ECUTestFramework() {
    // Limpiar suites
    for (auto& suite : suites_) {
        suite->teardown();
    }
}

inline void ECUTestFramework::run() {
    running_ = true;
    framework_start_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  ECU TEST FRAMEWORK - INICIANDO" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Ejecutar cada suite
    for (auto& suite : suites_) {
        std::cout << "[SUITE] " << suite->get_name() << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        auto tests = suite->get_tests();
        
        for (auto& test : tests) {
            test->set_name(test->get_name());
            run_single_test(test.get(), suite->get_name());
        }
        
        std::cout << "[RESULT] " << suite->get_pass_count() << " PASS, " 
                  << suite->get_fail_count() << " FAIL, "
                  << suite->get_skip_count() << " SKIP\n" << std::endl;
    }
    
    framework_end_ms_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    total_duration_us_ = (framework_end_ms_ - framework_start_ms_) * 1000;
    
    running_ = false;
    
    // Print summary
    print_summary();
}

inline void ECUTestFramework::run_single_test(BaseTest* test, const std::string& suite_name) {
    TestResultData result;
    result.suite = suite_name;
    result.name = test->get_name();
    result.start_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (context_) {
        test->set_context(context_);
        std::string reason;
        if (!context_->check_requirements(test->metadata(), reason)) {
            result.result = TestResult::SKIPPED;
            result.message = reason;
            results_.push_back(result);
            skip_count_++;
            total_tests_++;
            std::cout << "  [SKIP] " << test->get_name() 
                      << " => " << reason << std::endl;
            return;
        }
    }
    
    // Setup
    auto setup_start = std::chrono::high_resolution_clock::now();
    try {
        test->setup();
    } catch (const std::exception& e) {
        result.result = TestResult::FAIL;
        result.message = std::string("Setup failed: ") + e.what();
        results_.push_back(result);
        fail_count_++;
        total_tests_++;
        return;
    }
    auto setup_end = std::chrono::high_resolution_clock::now();
    result.setup_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        setup_end - setup_start).count();
    
    // Run
    auto run_start = std::chrono::high_resolution_clock::now();
    try {
        test->run();
        result.result = TestResult::PASS;
        result.message = "OK";
        pass_count_++;
    } catch (const AssertionError& e) {
        result.result = TestResult::FAIL;
        result.message = e.what();
        fail_count_++;
    } catch (const SkipTest& e) {
        result.result = TestResult::SKIPPED;
        result.message = e.what();
        skip_count_++;
    } catch (const std::exception& e) {
        result.result = TestResult::FAIL;
        result.message = std::string("Exception: ") + e.what();
        fail_count_++;
    }
    auto run_end = std::chrono::high_resolution_clock::now();
    result.duration_us_ = std::chrono::duration_cast<std::chrono::microseconds>(
        run_end - run_start).count();
    
    // Teardown
    auto teardown_start = std::chrono::high_resolution_clock::now();
    try {
        test->teardown();
    } catch (const std::exception& e) {
        // Solo warning, no falla el test
        std::cerr << "[WARN] Teardown failed for " << test->get_name() 
                  << ": " << e.what() << std::endl;
    }
    auto teardown_end = std::chrono::high_resolution_clock::now();
    result.teardown_time_us = std::chrono::duration_cast<std::chrono::microseconds>(
        teardown_end - teardown_start).count();
    
    result.end_timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Print resultado
    std::string status = (result.result == TestResult::PASS) ? "[PASS]" : "[FAIL]";
    std::cout << "  " << status << " " << test->get_name() 
              << " (" << result.duration_us_ << " us)" << std::endl;
    
    if (result.result == TestResult::FAIL) {
        std::cout << "        => " << result.message << std::endl;
    }
    
    results_.push_back(result);
    total_tests_++;
}

inline void ECUTestFramework::capture_system_metrics(SystemMetrics& metrics) {
    if (mode_manager_) {
        auto state = mode_manager_->get_state();
        metrics.mode = state.mode;
        metrics.power_factor = state.power_factor;
        metrics.torque_zero = state.torque_zero_commanded;
    }
}

inline std::string ECUTestFramework::get_result_string(TestResult result) {
    switch (result) {
        case TestResult::PASS: return "PASS";
        case TestResult::FAIL: return "FAIL";
        case TestResult::SKIPPED: return "SKIPPED";
        case TestResult::TIMEOUT: return "TIMEOUT";
        default: return "NOT_RUN";
    }
}

inline void ECUTestFramework::generate_json_report(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo crear reporte JSON: " << filename << std::endl;
        return;
    }
    
    file << "{\n";
    
    if (context_) {
        file << "  \"execution\": {\n";
        file << "    \"backend\": \"" << context_->backend_config.backend_type << "\",\n";
        file << "    \"if0\": \"" << context_->backend_config.if0 << "\",\n";
        file << "    \"if1\": \"" << context_->backend_config.if1 << "\",\n";
        file << "    \"policy\": \"" << (context_->security_policy ? context_->security_policy->name() : "UNKNOWN") << "\",\n";
        file << "    \"start_timestamp_ms\": " << framework_start_ms_ << ",\n";
        file << "    \"end_timestamp_ms\": " << framework_end_ms_ << "\n";
        file << "  },\n";
        
        file << "  \"system_info\": {\n";
        file << "    \"kernel\": \"" << context_->system_info.kernel_version << "\",\n";
        file << "    \"cpu_model\": \"" << context_->system_info.cpu_model << "\",\n";
        file << "    \"preempt_rt\": " << (context_->system_info.preempt_rt_detected ? "true" : "false") << ",\n";
        file << "    \"sched_fifo_allowed\": " << (context_->system_info.sched_fifo_available ? "true" : "false") << ",\n";
        file << "    \"cap_sys_nice\": " << (context_->system_info.has_cap_sys_nice ? "true" : "false") << ",\n";
        file << "    \"can_interface\": \"" << context_->system_info.can_interface_name << "\",\n";
        file << "    \"hmi_socket\": " << (context_->system_info.hmi_socket_exists ? "true" : "false") << ",\n";
        file << "    \"boot_time_ms\": " << context_->system_info.boot_time_ms << ",\n";
        file << "    \"test_start_ms\": " << context_->system_info.test_start_time_ms << "\n";
        file << "  },\n";
    }
    
    file << "  \"summary\": {\n";
    file << "    \"total_tests\": " << total_tests_ << ",\n";
    file << "    \"passed\": " << pass_count_ << ",\n";
    file << "    \"failed\": " << fail_count_ << ",\n";
    file << "    \"skipped\": " << skip_count_ << ",\n";
    file << "    \"duration_ms\": " << (total_duration_us_ / 1000) << "\n";
    file << "  },\n";
    
    // Skipped list
    file << "  \"skipped_tests\": [\n";
    bool first_skip = true;
    for (const auto& r : results_) {
        if (r.result != TestResult::SKIPPED) continue;
        if (!first_skip) file << ",\n";
        first_skip = false;
        file << "    {\"name\":\"" << r.name << "\",\"suite\":\"" << r.suite 
             << "\",\"reason\":\"" << r.message << "\"}";
    }
    if (!first_skip) file << "\n";
    file << "  ],\n";
    
    file << "  \"results\": [\n";
    for (size_t i = 0; i < results_.size(); ++i) {
        const auto& r = results_[i];
        file << "    {\n";
        file << "      \"name\": \"" << r.name << "\",\n";
        file << "      \"suite\": \"" << r.suite << "\",\n";
        file << "      \"result\": \"" << get_result_string(r.result) << "\",\n";
        file << "      \"message\": \"" << r.message << "\",\n";
        file << "      \"duration_us\": " << r.duration_us_ << ",\n";
        file << "      \"timestamp_ms\": " << r.start_timestamp_ms << "\n";
        file << "    }";
        if (i < results_.size() - 1) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    
    file.close();
    std::cout << "\n[REPORT] JSON: " << filename << std::endl;
}

inline void ECUTestFramework::generate_markdown_report(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] No se pudo crear reporte MD: " << filename << std::endl;
        return;
    }
    
    file << "# ECU Test Report\n\n";
    file << "## Summary\n\n";
    file << "| Metric | Value |\n";
    file << "|--------|-------|\n";
    file << "| Total Tests | " << total_tests_ << " |\n";
    file << "| Passed | " << pass_count_ << " |\n";
    file << "| Failed | " << fail_count_ << " |\n";
    file << "| Skipped | " << skip_count_ << " |\n";
    file << "| Duration | " << (total_duration_us_ / 1000000) << "s |\n\n";
    
    file << "## Results\n\n";
    file << "| Test | Suite | Result | Duration |\n";
    file << "|------|-------|--------|----------|\n";
    
    for (const auto& r : results_) {
        std::string status = (r.result == TestResult::PASS) ? "✅ PASS" : "❌ FAIL";
        file << "| " << r.name << " | " << r.suite << " | " 
             << status << " | " << (r.duration_us_ / 1000) << "ms |\n";
    }
    
    file << "\n## Details\n\n";
    
    // Agrupar por suite
    std::map<std::string, std::vector<const TestResultData*>> by_suite;
    for (const auto& r : results_) {
        by_suite[r.suite].push_back(&r);
    }
    
    for (const auto& pair : by_suite) {
        file << "### " << pair.first << "\n\n";
        
        for (const auto* r : pair.second) {
            if (r->result == TestResult::FAIL) {
                file << "#### " << r->name << "\n\n";
                file << "
```
\n";
                file << "Expected: " << r->expected << "\n";
                file << "Actual: " << r->actual << "\n";
                file << "Message: " << r->message << "\n";
                file << "
```
\n\n";
            }
        }
    }
    
    file.close();
    std::cout << "[REPORT] Markdown: " << filename << std::endl;
}

inline void ECUTestFramework::print_summary() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  TEST SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  Total:  " << total_tests_ << std::endl;
    std::cout << "  Passed: " << pass_count_ << std::endl;
    std::cout << "  Failed: " << fail_count_ << std::endl;
    std::cout << "  Skipped: " << skip_count_ << std::endl;
    std::cout << "  Duration: " << (total_duration_us_ / 1000000) << "s" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    if (fail_count_ > 0) {
        std::cout << "❌ ALGUNOS TESTS FALLARON\n" << std::endl;
    } else {
        std::cout << "✅ TODOS LOS TESTS PASARON\n" << std::endl;
    }
}

} // namespace testing
} // namespace ecu
