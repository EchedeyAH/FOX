#pragma once
/**
 * system_supervisor.hpp
 * Supervisor completo del sistema que integra todos los sensores
 * y determina el modo de operación del ECU.
 * 
 * Este módulo cierra el bucle de control y supervisión:
 * 
 * 1) RECEPCIÓN DE DATOS:
 *    - SOC batería (del BMS)
 *    - Temperatura batería
 *    - Temperatura motores (M1-M4)
 *    - Corriente batería
 *    - Posición acelerador
 *    - Posición freno
 *    - Estado comunicación CAN
 * 
 * 2) EVALUACIÓN DE CONDICIONES:
 *    - OK → LIMP_MODE: errores GRAVE
 *    - LIMP_MODE → SAFE_STOP: errores CRITICOS
 * 
 * 3) INTEGRACIÓN CON:
 *    - SystemModeManager (gestión de modos)
 *    - MotorTimeoutDetector (timeouts)
 *    - VoltageProtection (voltaje)
 *    - TelemetryPublisher (HMI)
 * 
 * Umbrales configurables:
 *    - SOC bajo: < 10% → CRITICO, < 20% → GRAVE
 *    - Temp batería: > 60°C → GRAVE, > 70°C → CRITICO
 *    - Temp motor: > 80°C → GRAVE, > 100°C → CRITICO
 *    - Voltaje: configurable via VoltageProtection
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>

#include "error_catalog.hpp"
#include "error_publisher.hpp"
#include "system_mode_manager.hpp"
#include "motor_timeout_detector.hpp"
#include "voltage_protection.hpp"

namespace common {

// ============================================================================
// CONSTANTES - UMBRALES DE PROTECCIÓN
// ============================================================================

// SOC (State of Charge) - porcentaje de batería
constexpr double SOC_CRITICAL_LOW = 10.0;   // <10% → SAFE_STOP
constexpr double SOC_WARNING_LOW = 20.0;     // <20% → LIMP_MODE

// Temperatura batería (°C)
constexpr double TEMP_BAT_WARNING = 60.0;    // >60°C → LIMP_MODE
constexpr double TEMP_BAT_CRITICAL = 70.0;   // >70°C → SAFE_STOP

// Temperatura motor (°C)
constexpr double TEMP_MOTOR_WARNING = 80.0;  // >80°C → LIMP_MODE
constexpr double TEMP_MOTOR_CRITICAL = 100.0; // >100°C → SAFE_STOP

// Corriente batería (A)
constexpr double CURRENT_MAX = 500.0;       // Corriente máxima
constexpr double CURRENT_WARNING = 400.0;    // >400A → LIMP_MODE

// Comunicación CAN
constexpr uint32_t CAN_TIMEOUT_MS = 1000;    // 1s sin mensajes CAN → WARNING

// Histéresis
constexpr double TEMP_HYSTERESIS = 5.0;      // 5°C de histéresis
constexpr double SOC_HYSTERESIS = 5.0;       // 5% de histéresis

// ============================================================================
// ESTRUCTURAS DE DATOS
// ============================================================================

/**
 * Estado de todos los sensores del sistema
 */
struct SensorState {
    // Batería
    double soc_percent = 50.0;              // State of Charge (0-100)
    double temp_battery_c = 25.0;            // Temperatura batería
    double voltage_pack_mv = 380000;         // Voltaje pack (mV)
    double current_battery_ma = 0;           // Corriente batería (mA)
    
    // Motores
    std::array<double, 4> temp_motors_c = {25.0, 25.0, 25.0, 25.0};  // Temp M1-M4
    
    // Pedales
    double accelerator_position = 0.0;       // 0.0 - 1.0
    double brake_position = 0.0;            // 0.0 - 1.0
    
    // Comunicación
    uint64_t last_can_msg_ms = 0;           // Timestamp último mensaje CAN
    bool can_connected = true;               // Estado conexión CAN
};

/**
 * Resultado de la evaluación de supervisión
 */
struct SupervisorResult {
    SystemMode recommended_mode;
    double power_limit_factor;
    bool torque_zero;
    std::string warning_message;
    
    // Flags de condiciones detectadas
    bool soc_low = false;
    bool soc_critical = false;
    bool temp_bat_high = false;
    bool temp_bat_critical = false;
    bool temp_motor_high = false;
    bool temp_motor_critical = false;
    bool voltage_fault = false;
    bool can_disconnected = false;
};

// ============================================================================
// SYSTEM SUPERVISOR
// ============================================================================

class SystemSupervisor {
public:
    SystemSupervisor();
    
    // ─────────────────────────────────────────────────────────────────────
    // API DE ACTUALIZACIÓN DE SENSORES
    // ─────────────────────────────────────────────────────────────────────
    
    // Actualizar estado de sensores (llamar desde thread de control)
    void update_sensors(const SensorState& sensors);
    
    // Actualizar individualmente
    void update_battery(double soc, double temp_c, double voltage_mv, double current_ma);
    void update_motor_temp(uint8_t motor_idx, double temp_c);
    void update_pedals(double accelerator, double brake);
    void update_can_status(bool connected, uint64_t last_msg_ms);
    
    // ─────────────────────────────────────────────────────────────────────
    // API DE EVALUACIÓN (llamar desde watchdog)
    // ─────────────────────────────────────────────────────────────────────
    
    // Evaluar condiciones y determinar modo
    SupervisorResult evaluate();
    
    // Obtener último resultado
    const SupervisorResult& get_last_result() const { return last_result_; }
    
    // Obtener estado de sensores
    const SensorState& get_sensors() const { return sensors_; }
    
    // ─────────────────────────────────────────────────────────────────────
    // APIs DE CONFIGURACIÓN
    // ─────────────────────────────────────────────────────────────────────
    
    // Configurar umbrales
    void set_soc_thresholds(double warning, double critical);
    void set_temp_battery_thresholds(double warning, double critical);
    void set_temp_motor_thresholds(double warning, double critical);
    
    // Habilitar/deshabilitar
    void enable(bool en) { enabled_.store(en); }
    bool is_enabled() const { return enabled_.load(); }
    
    // Reset
    void reset();

private:
    SensorState sensors_;
    SupervisorResult last_result_;
    std::atomic<bool> enabled_;
    
    // Umbrales configurables
    double soc_warning_ = SOC_WARNING_LOW;
    double soc_critical_ = SOC_CRITICAL_LOW;
    double temp_bat_warning_ = TEMP_BAT_WARNING;
    double temp_bat_critical_ = TEMP_BAT_CRITICAL;
    double temp_motor_warning_ = TEMP_MOTOR_WARNING;
    double temp_motor_critical_ = TEMP_MOTOR_CRITICAL;
    
    // Estado interno
    std::array<bool, 4> motor_high_temp_;  // Tracking temp motor alta
    
    // Métodos internos
    void check_soc(SupervisorResult& result);
    void check_battery_temp(SupervisorResult& result);
    void check_motor_temps(SupervisorResult& result);
    void check_can_connection(SupervisorResult& result);
    void determine_mode(SupervisorResult& result);
    void publish_warnings(const SupervisorResult& result);
};

// ============================================================================
// IMPLEMENTACIÓN
// ============================================================================

inline SystemSupervisor::SystemSupervisor() : enabled_(true) {
    motor_high_temp_.fill(false);
    reset();
}

inline void SystemSupervisor::reset() {
    sensors_ = SensorState{};
    last_result_ = SupervisorResult{};
    last_result_.recommended_mode = SystemMode::OK;
    last_result_.power_limit_factor = 1.0;
    last_result_.torque_zero = false;
}

inline void SystemSupervisor::update_sensors(const SensorState& sensors) {
    sensors_ = sensors;
}

inline void SystemSupervisor::update_battery(double soc, double temp_c, double voltage_mv, double current_ma) {
    sensors_.soc_percent = soc;
    sensors_.temp_battery_c = temp_c;
    sensors_.voltage_pack_mv = voltage_mv;
    sensors_.current_battery_ma = current_ma;
}

inline void SystemSupervisor::update_motor_temp(uint8_t motor_idx, double temp_c) {
    if (motor_idx < 4) {
        sensors_.temp_motors_c[motor_idx] = temp_c;
    }
}

inline void SystemSupervisor::update_pedals(double accelerator, double brake) {
    sensors_.accelerator_position = accelerator;
    sensors_.brake_position = brake;
}

inline void SystemSupervisor::update_can_status(bool connected, uint64_t last_msg_ms) {
    sensors_.can_connected = connected;
    sensors_.last_can_msg_ms = last_msg_ms;
}

inline SupervisorResult SystemSupervisor::evaluate() {
    SupervisorResult result;
    result.recommended_mode = SystemMode::OK;
    result.power_limit_factor = 1.0;
    result.torque_zero = false;
    
    if (!enabled_.load()) {
        last_result_ = result;
        return result;
    }
    
    // Verificar cada condición
    check_soc(result);
    check_battery_temp(result);
    check_motor_temps(result);
    check_can_connection(result);
    
    // Determinar modo final
    determine_mode(result);
    
    last_result_ = result;
    return result;
}

inline void SystemSupervisor::check_soc(SupervisorResult& result) {
    // SOC crítico (SAFE_STOP)
    if (sensors_.soc_percent < soc_critical_) {
        result.soc_critical = true;
        result.warning_message = "SOC crítico: " + std::to_string(sensors_.soc_percent) + "%";
        
        // Publicar error
        ecu::ErrorEvent evt;
        evt.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        evt.code = ecu::ErrorCode::BMS_SOC_LOW;
        evt.level = ecu::ErrorLevel::CRITICO;
        evt.group = ecu::ErrorGroup::BMS;
        evt.status = ecu::ErrorStatus::ACTIVO;
        evt.origin = "SUPERVISOR";
        evt.description = "SOC crítico: " + std::to_string(sensors_.soc_percent) + "%";
        evt.threshold_value = static_cast<int16_t>(sensors_.soc_percent);
        
        ecu::g_error_publisher.publish_event(evt);
    }
    // SOC bajo (LIMP_MODE)
    else if (sensors_.soc_percent < soc_warning_) {
        result.soc_low = true;
        result.warning_message = "SOC bajo: " + std::to_string(sensors_.soc_percent) + "%";
        
        LOG_WARN("SUPERVISOR", "SOC bajo: " + std::to_string(sensors_.soc_percent) + "%");
    }
}

inline void SystemSupervisor::check_battery_temp(SupervisorResult& result) {
    // Temperatura batería crítica
    if (sensors_.temp_battery_c > temp_bat_critical_) {
        result.temp_bat_critical = true;
        result.warning_message = "Temp batería crítica: " + std::to_string(sensors_.temp_battery_c) + "°C";
        
        ecu::ErrorEvent evt;
        evt.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        evt.code = ecu::ErrorCode::BMS_TEMP_HIGH;
        evt.level = ecu::ErrorLevel::CRITICO;
        evt.group = ecu::ErrorGroup::BMS;
        evt.status = ecu::ErrorStatus::ACTIVO;
        evt.origin = "SUPERVISOR";
        evt.description = "Temp batería crítica: " + std::to_string(sensors_.temp_battery_c) + "°C";
        
        ecu::g_error_publisher.publish_event(evt);
    }
    // Temperatura batería alta
    else if (sensors_.temp_battery_c > temp_bat_warning_) {
        result.temp_bat_high = true;
        result.warning_message = "Temp batería alta: " + std::to_string(sensors_.temp_battery_c) + "°C";
        
        LOG_WARN("SUPERVISOR", "Temp batería alta: " + std::to_string(sensors_.temp_battery_c) + "°C");
    }
}

inline void SystemSupervisor::check_motor_temps(SupervisorResult& result) {
    bool any_critical = false;
    bool any_high = false;
    
    for (int i = 0; i < 4; ++i) {
        double temp = sensors_.temp_motors_c[i];
        
        if (temp > temp_motor_critical_) {
            any_critical = true;
            motor_high_temp_[i] = true;
            
            // Error crítico por motor
            ecu::ErrorEvent evt;
            evt.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            ErrorCode code = (i == 0) ? ecu::ErrorCode::M1_TEMP_HIGH :
                            (i == 1) ? ecu::ErrorCode::M2_TEMP_HIGH :
                            (i == 2) ? ecu::ErrorCode::M3_TEMP_HIGH :
                            ecu::ErrorCode::M4_TEMP_HIGH;
            
            evt.code = code;
            evt.level = ecu::ErrorLevel::CRITICO;
            evt.group = ecu::ErrorGroup::MOTOR;
            evt.status = ecu::ErrorStatus::ACTIVO;
            evt.origin = "M" + std::to_string(i + 1);
            evt.description = "Temp motor crítica: " + std::to_string(temp) + "°C";
            
            ecu::g_error_publisher.publish_event(evt);
        }
        else if (temp > temp_motor_warning_) {
            any_high = true;
            motor_high_temp_[i] = true;
        }
        else {
            // Recovery - temperatura normal
            if (motor_high_temp_[i]) {
                motor_high_temp_[i] = false;
                
                LOG_INFO("SUPERVISOR", "Temp motor M" + std::to_string(i+1) + " normalizada");
            }
        }
    }
    
    result.temp_motor_critical = any_critical;
    result.temp_motor_high = any_high;
    
    if (any_critical) {
        result.warning_message = "Temperatura motor crítica";
    }
    else if (any_high) {
        result.warning_message = "Temperatura motor alta";
    }
}

inline void SystemSupervisor::check_can_connection(SupervisorResult& result) {
    if (!sensors_.can_connected) {
        result.can_disconnected = true;
        result.warning_message = "CAN desconectado";
        
        LOG_ERROR("SUPERVISOR", "CAN desconectado");
    }
}

inline void SystemSupervisor::determine_mode(SupervisorResult& result) {
    // Orden de prioridad (más crítico primero):
    // 1. Cualquier condición CRÍTICA → SAFE_STOP
    // 2. Cualquier condición de WARNING → LIMP_MODE
    // 3. Todo OK → OK
    
    if (result.soc_critical || result.temp_bat_critical || result.temp_motor_critical) {
        // CRÍTICO → SAFE_STOP
        result.recommended_mode = SystemMode::SAFE_STOP;
        result.power_limit_factor = 0.0;
        result.torque_zero = true;
    }
    else if (result.soc_low || result.temp_bat_high || result.temp_motor_high || result.can_disconnected) {
        // WARNING → LIMP_MODE
        result.recommended_mode = SystemMode::LIMP_MODE;
        
        // Determinar factor según la condición más severa
        if (result.soc_low) {
            result.power_limit_factor = 0.3;  // 30% potencia
        }
        else if (result.temp_bat_high || result.temp_motor_high) {
            result.power_limit_factor = 0.5;  // 50% potencia
        }
        else {
            result.power_limit_factor = 0.7;  // 70% potencia
        }
    }
    else {
        // Todo OK
        result.recommended_mode = SystemMode::OK;
        result.power_limit_factor = 1.0;
        result.torque_zero = false;
    }
}

inline void SystemSupervisor::set_soc_thresholds(double warning, double critical) {
    soc_warning_ = warning;
    soc_critical_ = critical;
}

inline void SystemSupervisor::set_temp_battery_thresholds(double warning, double critical) {
    temp_bat_warning_ = warning;
    temp_bat_critical_ = critical;
}

inline void SystemSupervisor::set_temp_motor_thresholds(double warning, double critical) {
    temp_motor_warning_ = warning;
    temp_motor_critical_ = critical;
}

// Instancia global
inline SystemSupervisor& get_system_supervisor() {
    static SystemSupervisor supervisor;
    return supervisor;
}

} // namespace common
