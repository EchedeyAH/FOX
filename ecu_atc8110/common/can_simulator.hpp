#pragma once
/**
 * can_simulator.hpp
 * Simulador de mensajes CAN para pruebas del sistema ECU
 * 
 * Permite simular:
 * - Mensajes de motores (respuestas 0x281-0x284)
 * - Mensajes del BMS (voltaje, SOC, temperatura)
 * - Pérdida de comunicación (timeouts)
 * - Condiciones de error (voltaje bajo, temperatura alta)
 * 
 * Modos de simulación:
 * - NORMAL: Mensajes normales, sin errores
 * - MOTOR_TIMEOUT: Simular pérdida de comunicación motores
 * - LOW_VOLTAGE: Simular voltaje bajo
 * - HIGH_TEMP: Simular temperatura alta
 * - BMS_ERROR: Simular error BMS
 * - MANUAL: Control manual de mensajes
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>
#include <random>
#include <functional>
#include <thread>
#include <vector>

#include "logging.hpp"
#include "error_catalog.hpp"

namespace common {

// ============================================================================
// CONSTANTES
// ============================================================================

// IDs CAN simulados
constexpr uint16_t SIM_MOTOR_1_RESP = 0x281;
constexpr uint16_t SIM_MOTOR_2_RESP = 0x282;
constexpr uint16_t SIM_MOTOR_3_RESP = 0x283;
constexpr uint16_t SIM_MOTOR_4_RESP = 0x284;
constexpr uint16_t SIM_BMS_MSG = 0x04D;

// Períodos de simulación
constexpr uint32_t SIM_MOTOR_PERIOD_MS = 50;   // 50ms entre mensajes motor
constexpr uint32_t SIM_BMS_PERIOD_MS = 100;     // 100ms entre mensajes BMS

// ============================================================================
// ENUMS
// ============================================================================

/**
 * Modo de simulación
 */
enum class SimulationMode {
    DISABLED = 0,    // Simulador apagado
    NORMAL = 1,      // Operación normal sin errores
    MOTOR_TIMEOUT = 2,    // Pérdida de comunicación motores
    LOW_VOLTAGE = 3,     // Voltaje batería bajo
    HIGH_TEMP = 4,       // Temperatura alta
    BMS_ERROR = 5,       // Error BMS
    MANUAL = 6           // Control manual
};

/**
 * Mensaje CAN simulado
 */
struct SimCanMessage {
    uint16_t id;
    uint64_t timestamp_ms;
    std::vector<uint8_t> data;
    bool valid;
};

// ============================================================================
// CONFIGURACIÓN DE SIMULACIÓN
// ============================================================================

/**
 * Parámetros configurables de simulación
 */
struct SimulationConfig {
    // Batería
    double vbat_mv = 380000;          // Voltaje pack (mV)
    double soc_percent = 50.0;         // State of Charge (%)
    double temp_battery_c = 25.0;      // Temperatura batería (°C)
    double current_ma = 0;             // Corriente (mA)
    
    // Motores
    std::array<double, 4> motor_temps_c = {30.0, 30.0, 30.0, 30.0};
    std::array<double, 4> motor_rpm = {0.0, 0.0, 0.0, 0.0};
    std::array<double, 4> motor_torque_nm = {0.0, 0.0, 0.0, 0.0};
    std::array<bool, 4> motor_alive = {true, true, true, true};
    
    // Configuración de errores
    double error_vbat_mv = 45000;      // Voltaje para modo LOW_VOLTAGE
    double error_temp_c = 90.0;       // Temperatura para modo HIGH_TEMP
    uint32_t error_timeout_ms = 2000;  // Timeout para modo MOTOR_TIMEOUT
    
    // Variaciones (ruido)
    bool enable_noise = false;
    double noise_voltage_mv = 1000;    // Ruido voltaje ±mV
    double noise_temp_c = 2.0;         // Ruido temperatura ±°C
    double noise_current_ma = 100;    // Ruido corriente ±mA
};

// ============================================================================
// CAN SIMULATOR
// ============================================================================

class CanSimulator {
public:
    CanSimulator();
    
    // ─────────────────────────────────────────────────────────────────────
    // CONTROL DE SIMULACIÓN
    // ─────────────────────────────────────────────────────────────────────
    
    // Iniciar simulador
    void start();
    
    // Detener simulador
    void stop();
    
    // Establecer modo de simulación
    void set_mode(SimulationMode mode);
    
    // Obtener modo actual
    SimulationMode get_mode() const { return mode_.load(); }
    
    // Verificar si está corriendo
    bool is_running() const { return running_.load(); }
    
    // ─────────────────────────────────────────────────────────────────────
    // CONFIGURACIÓN
    // ─────────────────────────────────────────────────────────────────────
    
    // Configurar parámetros
    void set_config(const SimulationConfig& config);
    SimulationConfig get_config() const { return config_; }
    
    // Configurar callbacks para mensajes CAN
    using MessageCallback = std::function<void(const SimCanMessage&)>;
    void set_motor_callback(MessageCallback cb) { motor_callback_ = cb; }
    void set_bms_callback(MessageCallback cb) { bms_callback_ = cb; }
    
    // ─────────────────────────────────────────────────────────────────────
    // CONTROL MANUAL (para modo MANUAL)
    // ─────────────────────────────────────────────────────────────────────
    
    // Inyectar mensaje de motor manualmente
    void inject_motor_message(uint8_t motor_idx, const std::vector<uint8_t>& data);
    
    // Inyectar mensaje BMS manualmente
    void inject_bms_message(const std::vector<uint8_t>& data);
    
    // Provocar timeout de motor
    void trigger_motor_timeout(uint8_t motor_idx);
    
    // Recuperar motor de timeout
    void recover_motor(uint8_t motor_idx);
    
    // ─────────────────────────────────────────────────────────────────────
    // CONSULTAS
    // ─────────────────────────────────────────────────────────────────────
    
    // Obtener último mensaje generado
    const SimCanMessage& get_last_motor_msg() const { return last_motor_msg_; }
    const SimCanMessage& get_last_bms_msg() const { return last_bms_msg_; }
    
    // Obtener contador de mensajes
    uint32_t get_motor_msg_count() const { return motor_msg_count_.load(); }
    uint32_t get_bms_msg_count() const { return bms_msg_count_.load(); }

private:
    // Estado
    std::atomic<bool> running_;
    std::atomic<SimulationMode> mode_;
    SimulationConfig config_;
    
    // Callbacks
    MessageCallback motor_callback_;
    MessageCallback bms_callback_;
    
    // Thread
    std::thread sim_thread_;
    
    // Contadores
    std::atomic<uint32_t> motor_msg_count_;
    std::atomic<uint32_t> bms_msg_count_;
    
    // Últimos mensajes
    SimCanMessage last_motor_msg_;
    SimCanMessage last_bms_msg_;
    
    // Estado de timeouts
    std::array<bool, 4> motor_timeout_active_;
    
    // Random para ruido
    std::random_device rd_;
    std::mt19937 gen_;
    std::normal_distribution<> dist_voltage_;
    std::normal_distribution<> dist_temp_;
    std::normal_distribution<> dist_current_;
    
    // Métodos internos
    void simulation_loop();
    void generate_motor_messages();
    void generate_bms_message();
    void apply_mode_effects();
    double add_noise(double value);
};

// ============================================================================
// IMPLEMENTACIÓN
// ============================================================================

inline CanSimulator::CanSimulator() 
    : running_(false)
    , mode_(SimulationMode::DISABLED)
    , motor_msg_count_(0)
    , bms_msg_count_(0)
    , gen_(rd_())
    , dist_voltage_(0.0, 1.0)
    , dist_temp_(0.0, 1.0)
    , dist_current_(0.0, 1.0)
{
    motor_timeout_active_.fill(false);
    
    last_motor_msg_ = {0, 0, {}, false};
    last_bms_msg_ = {0, 0, {}, false};
}

inline void CanSimulator::start() {
    if (running_.load()) return;
    
    running_ = true;
    mode_.store(SimulationMode::NORMAL);
    
    sim_thread_ = std::thread(&CanSimulator::simulation_loop, this);
    
    LOG_INFO("CAN_SIM", "Simulador iniciado en modo NORMAL");
}

inline void CanSimulator::stop() {
    if (!running_.load()) return;
    
    running_ = false;
    mode_.store(SimulationMode::DISABLED);
    
    if (sim_thread_.joinable()) {
        sim_thread_.join();
    }
    
    LOG_INFO("CAN_SIM", "Simulador detenido");
}

inline void CanSimulator::set_mode(SimulationMode mode) {
    mode_.store(mode);
    
    const char* mode_names[] = {"DISABLED", "NORMAL", "MOTOR_TIMEOUT", 
                                  "LOW_VOLTAGE", "HIGH_TEMP", "BMS_ERROR", "MANUAL"};
    
    LOG_INFO("CAN_SIM", std::string("Modo: ") + mode_names[static_cast<int>(mode)]);
    
    // Aplicar efectos según modo
    apply_mode_effects();
}

inline void CanSimulator::set_config(const SimulationConfig& config) {
    config_ = config;
}

inline void CanSimulator::apply_mode_effects() {
    SimulationMode mode = mode_.load();
    
    switch (mode) {
        case SimulationMode::LOW_VOLTAGE:
            // Reducir voltaje
            config_.vbat_mv = config_.error_vbat_mv;
            break;
            
        case SimulationMode::HIGH_TEMP:
            // Aumentar temperaturas
            config_.motor_temps_c = {config_.error_temp_c, config_.error_temp_c, 
                                     config_.error_temp_c, config_.error_temp_c};
            config_.temp_battery_c = config_.error_temp_c;
            break;
            
        case SimulationMode::MOTOR_TIMEOUT:
            // Marcar todos los motores como timeout
            motor_timeout_active_.fill(true);
            break;
            
        case SimulationMode::NORMAL:
        default:
            // Restaurar valores normales
            config_.vbat_mv = 380000;
            config_.motor_temps_c = {30.0, 30.0, 30.0, 30.0};
            config_.temp_battery_c = 25.0;
            motor_timeout_active_.fill(false);
            break;
    }
}

inline void CanSimulator::inject_motor_message(uint8_t motor_idx, const std::vector<uint8_t>& data) {
    if (motor_idx >= 4) return;
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    SimCanMessage msg;
    msg.id = SIM_MOTOR_1_RESP + motor_idx;
    msg.timestamp_ms = now;
    msg.data = data;
    msg.valid = true;
    
    last_motor_msg_ = msg;
    motor_msg_count_++;
    
    if (motor_callback_) {
        motor_callback_(msg);
    }
}

inline void CanSimulator::inject_bms_message(const std::vector<uint8_t>& data) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    SimCanMessage msg;
    msg.id = SIM_BMS_MSG;
    msg.timestamp_ms = now;
    msg.data = data;
    msg.valid = true;
    
    last_bms_msg_ = msg;
    bms_msg_count_++;
    
    if (bms_callback_) {
        bms_callback_(msg);
    }
}

inline void CanSimulator::trigger_motor_timeout(uint8_t motor_idx) {
    if (motor_idx < 4) {
        motor_timeout_active_[motor_idx] = true;
        LOG_WARN("CAN_SIM", "Timeout motor M" + std::to_string(motor_idx + 1) + " activado");
    }
}

inline void CanSimulator::recover_motor(uint8_t motor_idx) {
    if (motor_idx < 4) {
        motor_timeout_active_[motor_idx] = false;
        LOG_INFO("CAN_SIM", "Motor M" + std::to_string(motor_idx + 1) + " recuperado");
    }
}

inline void CanSimulator::simulation_loop() {
    auto last_motor_time = std::chrono::steady_clock::now();
    auto last_bms_time = std::chrono::steady_clock::now();
    
    while (running_.load()) {
        auto now = std::chrono::steady_clock::now();
        
        // Generar mensajes de motores cada 50ms
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_motor_time).count() >= SIM_MOTOR_PERIOD_MS) {
            generate_motor_messages();
            last_motor_time = now;
        }
        
        // Generar mensaje BMS cada 100ms
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_bms_time).count() >= SIM_BMS_PERIOD_MS) {
            generate_bms_message();
            last_bms_time = now;
        }
        
        // Pequeña espera
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

inline void CanSimulator::generate_motor_messages() {
    SimulationMode mode = mode_.load();
    if (mode == SimulationMode::DISABLED) return;
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (int i = 0; i < 4; ++i) {
        // Verificar si motor está en timeout
        if (motor_timeout_active_[i]) {
            continue;  // No generar mensaje
        }
        
        // Generar mensaje de temperatura (CCP_MONITOR1 = 0x03)
        SimCanMessage msg;
        msg.id = SIM_MOTOR_1_RESP + i;
        msg.timestamp_ms = now;
        msg.valid = true;
        
        // Datos: cmd + temp_motor + temp_inverter
        double temp = config_.motor_temps_c[i];
        if (config_.enable_noise) {
            temp = add_noise(temp);
        }
        
        msg.data = {
            0x03,                                     // CCP_MONITOR1
            static_cast<uint8_t>(temp),              // Temp motor
            static_cast<uint8_t>(temp - 5),         // Temp inverter (un poco menor)
            0x00, 0x00                               // Padding
        };
        
        last_motor_msg_ = msg;
        motor_msg_count_++;
        
        if (motor_callback_) {
            motor_callback_(msg);
        }
    }
}

inline void CanSimulator::generate_bms_message() {
    SimulationMode mode = mode_.load();
    if (mode == SimulationMode::DISABLED) return;
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    SimCanMessage msg;
    msg.id = SIM_BMS_MSG;
    msg.timestamp_ms = now;
    msg.valid = true;
    
    // Aplicar ruido si está habilitado
    double vbat = config_.vbat_mv;
    double soc = config_.soc_percent;
    double temp = config_.temp_battery_c;
    double current = config_.current_ma;
    
    if (config_.enable_noise) {
        vbat = add_noise(vbat);
        temp = add_noise(temp);
        current = add_noise(current);
    }
    
    // Datos BMS: voltaje, SOC, temperatura, corriente
    uint16_t vbat_v = static_cast<uint16_t>(vbat / 1000);  // V
    uint8_t soc_byte = static_cast<uint8_t>(soc * 2.55);  // 0-255
    uint8_t temp_byte = static_cast<uint8_t>(temp + 40);  // Offset -40
    
    msg.data = {
        static_cast<uint8_t>(vbat_v >> 8),    // Voltaje high
        static_cast<uint8_t>(vbat_v & 0xFF), // Voltaje low
        soc_byte,                             // SOC
        temp_byte,                            // Temperatura
        static_cast<uint8_t>(current >> 8),  // Corriente high
        static_cast<uint8_t>(current & 0xFF) // Corriente low
    };
    
    last_bms_msg_ = msg;
    bms_msg_count_++;
    
    if (bms_callback_) {
        bms_callback_(msg);
    }
}

inline double CanSimulator::add_noise(double value) {
    // Ruido gaussiano simple
    double noise = dist_voltage_(gen_);
    return value + noise;
}

// Instancia global
inline CanSimulator& get_can_simulator() {
    static CanSimulator simulator;
    return simulator;
}

} // namespace common
