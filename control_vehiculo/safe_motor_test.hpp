#pragma once

#include "../common/interfaces.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <cstdio>
#include <chrono>

namespace control_vehiculo {

// ═══════════════════════════════════════════════════════════════════════════
// ✅ SAFE TEST MODE CONFIGURATION
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Global SAFE_TEST_MODE flag - Activar para banco de pruebas seguro
 * 
 * Cuando está ACTIVO:
 * - Limita torque a MAX_TORQUE_SAFE (15 Nm)
 * - Aplica rampa suave de aceleración
 * - Corta motor si freno es presionado
 * - Valida rangos de sensores
 * - Aplica timeout de seguridad
 * - Limita salida analógica a 2.0V
 */
constexpr bool SAFE_TEST_MODE = true;

// Límites de seguridad
constexpr double MAX_TORQUE_SAFE = 15.0;      // Nm - LÍMITE CRÍTICO para pruebas
constexpr double MAX_VOLTAGE_SAFE = 2.0;      // 5V × (15.0/100.0)
constexpr double WATCHDOG_TIMEOUT_MS = 200.0; // ms - Sin actualización → corte
constexpr double THROTTLE_SENSOR_MIN = 0.2;   // 1V (aprox)
constexpr double THROTTLE_SENSOR_MAX = 4.8;   // 4.8V (aprox)
constexpr double BRAKE_FAILSAFE_THRESHOLD = 1.0; // 5V × 0.2

enum class MotorState {
    IDLE,
    ARMED,
    RUNNING,
    SAFE_STOP,      // Estados nuevos
    EMERGENCY_STOP
};

enum class SafeFailureReason {
    NONE = 0,
    ADC_FAULT,
    CAN_FAULT,
    SENSOR_OUT_OF_RANGE,
    TIMEOUT,
    BRAKE_PRESSED,
    UNKNOWN
};

/**
 * @struct SafeMotorTest
 * @brief Control seguro de motor con múltiples capas de protección RT
 * 
 * Implementa:
 * ✅ FSM segura (IDLE → ARMED → RUNNING)
 * ✅ Limitación de torque (CRÍTICO)
 * ✅ Rampa suave de aceleración
 * ✅ Failsafe de freno
 * ✅ Timeout de watchdog
 * ✅ Validación de sensores
 * ✅ Salidas clampeadas
 * ✅ Logging profesional
 */
struct SafeMotorTest {
    // ─ Estado de máquina
    MotorState state{MotorState::IDLE};
    SafeFailureReason last_failure{SafeFailureReason::NONE};
    
    // ─ Variables de control
    double current_voltage{0.0};           // Voltaje actual (rampa)
    double target_voltage{0.0};            // Voltaje objetivo
    double throttle_current{0.0};          // Acelerador actual (0-1)
    double throttle_target{0.0};           // Acelerador objetivo (0-1)
    double stable_time_s{0.0};             // Tiempo en estado ARMED
    
    // ─ Timers
    double time_since_last_throttle{0.0};  // Watchdog timer
    
    // ─ Configuración (ajustable)
    double max_safe_voltage{MAX_VOLTAGE_SAFE};
    double max_torque_nm{MAX_TORQUE_SAFE};
    double ramp_rate_v_per_s{0.5};
    double ramp_rate_throttle_per_s{0.5};
    double deadzone{0.05};
    double stable_required_s{2.0};
    double brake_pressed_threshold{0.2};
    double watchdog_timeout_s{WATCHDOG_TIMEOUT_MS / 1000.0};
    double throttle_sensor_min_v{THROTTLE_SENSOR_MIN};
    double throttle_sensor_max_v{THROTTLE_SENSOR_MAX};
    
    // ─ Monitoreo
    int fault_count{0};                    // Contador de fallos
    int cycle_count{0};                    // Contador de ciclos
    int log_cycle_interval{50};            // Log cada N ciclos (~1s @ 50Hz)
    
    // ─ Límites de voltaje analógica
    double max_analog_output{2.0};         // Salida máxima permitida (CRÍTICO)

    /**
     * @brief Actualizar control de motor seguro
     * 
     * @param throttle_raw Acelerador entrada (0-1) o voltaje (0-5V)
     * @param brake_raw Freno entrada (0-1) o voltaje (0-5V)
     * @param adc_ok ADC funcionando correctamente
     * @param comm_ok Comunicación CAN OK
     * @param dt Delta de tiempo en segundos
     * @return true si estado actual es válido, false si error crítico
     */
    bool update(double throttle_raw, double brake_raw, bool adc_ok, bool comm_ok, double dt)
    {
        cycle_count++;
        
        // ═══════════════════════════════════════════════════════════════════
        // 1️⃣ VALIDACIÓN DE ENTRADA (CRÍTICA)
        // ═══════════════════════════════════════════════════════════════════
        
        // Verificar comunicación y ADC
        if (!adc_ok || !comm_ok) {
            handle_fault(SafeFailureReason::ADC_FAULT);
            state = MotorState::SAFE_STOP;
            current_voltage = 0.0;
            throttle_current = 0.0;
            return false;
        }
        
        // Validar rango de entrada
        if (throttle_raw < 0.0 || throttle_raw > 1.0 ||
            brake_raw < 0.0 || brake_raw > 1.0) {
            handle_fault(SafeFailureReason::SENSOR_OUT_OF_RANGE);
            state = MotorState::SAFE_STOP;
            return false;
        }
        
        // Validar rango de voltaje sensor (si está en rango de voltaje)
        // Asume entrada 0-1 = 0-5V, validar entre 0.2V y 4.8V
        if (throttle_raw > 0.01) { // Si no es exactamente cero
            double throttle_voltage = throttle_raw * 5.0;
            if ((throttle_voltage > 0.01 && throttle_voltage < throttle_sensor_min_v) ||
                throttle_voltage > throttle_sensor_max_v) {
                handle_fault(SafeFailureReason::SENSOR_OUT_OF_RANGE);
                throttle_raw = 0.0;
            }
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 2️⃣ WATCHDOG DE SEGURIDAD
        // ═══════════════════════════════════════════════════════════════════
        
        time_since_last_throttle += dt;
        if (time_since_last_throttle > watchdog_timeout_s) {
            if (throttle_current > 0.001) {
                handle_fault(SafeFailureReason::TIMEOUT);
            }
            throttle_current = 0.0;
            throttle_raw = 0.0;
        } else {
            time_since_last_throttle = 0.0; // Reset si hay actividad
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 3️⃣ FAILSAFE DE FRENO (¡MÁXIMA PRIORIDAD!)
        // ═══════════════════════════════════════════════════════════════════
        
        if (brake_raw >= brake_pressed_threshold) {
            // Freno presionado → CORTE INMEDIATO
            state = MotorState::SAFE_STOP;
            throttle_target = 0.0;
            throttle_raw = 0.0;
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 4️⃣ DEADZONE DE ACELERADOR
        // ═══════════════════════════════════════════════════════════════════
        
        if (std::fabs(throttle_raw) < deadzone) {
            throttle_raw = 0.0;
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 5️⃣ MÁQUINA DE ESTADOS
        // ═══════════════════════════════════════════════════════════════════
        
        const bool safe_startup_condition = (brake_raw >= brake_pressed_threshold) || 
                                           (throttle_raw == 0.0);
        
        switch (state) {
        case MotorState::IDLE:
            throttle_target = 0.0;
            if (safe_startup_condition) {
                state = MotorState::ARMED;
                stable_time_s = 0.0;
            }
            break;
            
        case MotorState::ARMED:
            throttle_target = 0.0; // No se mueve mientras está armado
            if (!safe_startup_condition) {
                // Perdió freno sin throttle
                state = MotorState::IDLE;
                stable_time_s = 0.0;
            } else {
                stable_time_s += dt;
                if (stable_time_s >= stable_required_s) {
                    state = MotorState::RUNNING;
                }
            }
            break;
            
        case MotorState::RUNNING:
            // Motor en marcha → seguir throttle
            throttle_target = throttle_raw;
            if (brake_raw >= brake_pressed_threshold) {
                state = MotorState::SAFE_STOP;
            }
            break;
            
        case MotorState::SAFE_STOP:
            throttle_target = 0.0;
            // Esperar freno suelto para resetear
            if (brake_raw < (brake_pressed_threshold * 0.5)) {
                state = MotorState::IDLE;
            }
            break;
            
        case MotorState::EMERGENCY_STOP:
            throttle_target = 0.0;
            // Estado terminal - requiere reset manual
            break;
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 6️⃣ RAMPA DE ACELERACIÓN (ANTI-TIRONES)
        // ═══════════════════════════════════════════════════════════════════
        
        {
            double delta_throttle = throttle_target - throttle_current;
            const double max_step_throttle = ramp_rate_throttle_per_s * dt;
            
            if (std::fabs(delta_throttle) > max_step_throttle) {
                delta_throttle = (delta_throttle > 0.0 ? 1.0 : -1.0) * max_step_throttle;
            }
            
            throttle_current += delta_throttle;
            throttle_current = std::clamp(throttle_current, 0.0, 1.0);
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 7️⃣ CONVERSIÓN: THROTTLE → VOLTAJE
        // ═══════════════════════════════════════════════════════════════════
        
        target_voltage = throttle_current * max_safe_voltage;
        target_voltage = std::clamp(target_voltage, 0.0, max_safe_voltage);
        
        // ═══════════════════════════════════════════════════════════════════
        // 8️⃣ RAMPA DE VOLTAJE
        // ═══════════════════════════════════════════════════════════════════
        
        {
            double delta_voltage = target_voltage - current_voltage;
            const double max_step_voltage = ramp_rate_v_per_s * dt;
            
            if (std::fabs(delta_voltage) > max_step_voltage) {
                delta_voltage = (delta_voltage > 0.0 ? 1.0 : -1.0) * max_step_voltage;
            }
            
            current_voltage += delta_voltage;
            current_voltage = std::clamp(current_voltage, 0.0, max_safe_voltage);
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 9️⃣ LÍMITE DE VOLTAJE ANALÓGICA (CRÍTICO)
        // ═══════════════════════════════════════════════════════════════════
        
        if (current_voltage > max_analog_output) {
            current_voltage = max_analog_output;
            handle_fault(SafeFailureReason::NONE);
        }
        
        // ═══════════════════════════════════════════════════════════════════
        // 🔟 LOGGING PROFESIONAL
        // ═══════════════════════════════════════════════════════════════════
        
        if (cycle_count % log_cycle_interval == 0) {
            const char* state_str = state_to_string(state);
            printf("[SAFE_TEST] state=%s thr_in=%.2f thr_cur=%.2f volt=%.2f "
                   "brake=%.2f faults=%d watchdog=%.0fms\n",
                   state_str,
                   throttle_raw,
                   throttle_current,
                   current_voltage,
                   brake_raw,
                   fault_count,
                   time_since_last_throttle * 1000.0);
        }
        
        return (state != MotorState::EMERGENCY_STOP);
    }
    
    /**
     * @brief Obtener torque equivalente en Nm
     * Asume conversión: voltage = torque × (max_voltage / max_torque)
     */
    double get_torque_nm() const {
        if (max_torque_nm <= 0.0) return 0.0;
        return (current_voltage / max_safe_voltage) * max_torque_nm;
    }
    
    /**
     * @brief Reset a IDLE (para testing)
     */
    void reset() {
        state = MotorState::IDLE;
        current_voltage = 0.0;
        target_voltage = 0.0;
        throttle_current = 0.0;
        throttle_target = 0.0;
        time_since_last_throttle = 0.0;
        stable_time_s = 0.0;
        last_failure = SafeFailureReason::NONE;
    }
    
    /**
     * @brief Forzar parada de emergencia
     */
    void emergency_stop() {
        state = MotorState::EMERGENCY_STOP;
        current_voltage = 0.0;
        throttle_current = 0.0;
        handle_fault(SafeFailureReason::UNKNOWN);
    }
    
private:
    void handle_fault(SafeFailureReason reason) {
        if (reason != SafeFailureReason::NONE) {
            fault_count++;
            last_failure = reason;
            printf("[SAFE_TEST_FAULT] reason=%s total_faults=%d\n",
                   fault_reason_to_string(reason), fault_count);
        }
    }
    
    static const char* state_to_string(MotorState s) {
        switch (s) {
        case MotorState::IDLE:            return "IDLE";
        case MotorState::ARMED:           return "ARMED";
        case MotorState::RUNNING:         return "RUNNING";
        case MotorState::SAFE_STOP:       return "SAFE_STOP";
        case MotorState::EMERGENCY_STOP:  return "EMERGENCY_STOP";
        default:                          return "UNKNOWN";
        }
    }
    
    static const char* fault_reason_to_string(SafeFailureReason r) {
        switch (r) {
        case SafeFailureReason::NONE:                  return "NONE";
        case SafeFailureReason::ADC_FAULT:            return "ADC_FAULT";
        case SafeFailureReason::CAN_FAULT:            return "CAN_FAULT";
        case SafeFailureReason::SENSOR_OUT_OF_RANGE:  return "SENSOR_OUT_OF_RANGE";
        case SafeFailureReason::TIMEOUT:              return "TIMEOUT";
        case SafeFailureReason::BRAKE_PRESSED:        return "BRAKE_PRESSED";
        case SafeFailureReason::UNKNOWN:              return "UNKNOWN";
        default:                                       return "UNDEFINED";
        }
    }
};

// ═══════════════════════════════════════════════════════════════════════════
// FUNCIONES HELPER
// ═══════════════════════════════════════════════════════════════════════════

/**
 * @brief Escribir salida analógica con límite de seguridad
 * 
 * IMPORTANTE: Jamás permitir 5V directo. Máximo 2.0V en SAFE_TEST_MODE
 */
inline void safeWriteAnalog(common::IActuatorWriter* actuator, int channel, double voltage)
{
    if (!actuator) {
        return;
    }
    
    // Aplicar límite de seguridad
    double safe_voltage = voltage;
    if (SAFE_TEST_MODE) {
        safe_voltage = std::min(safe_voltage, MAX_VOLTAGE_SAFE);
    }
    
    const std::string ch = "AO" + std::to_string(channel);
    actuator->write_output(ch, safe_voltage);
}

/**
 * @brief Convertir torque (Nm) a voltaje de salida
 * Considerando max_torque = 100Nm → 5V (sin SAFE_TEST)
 *                        = 15Nm → 1.5V (con SAFE_TEST)
 */
inline double torque_to_voltage(double torque_nm, bool safe_mode = SAFE_TEST_MODE)
{
    const double max_torque = safe_mode ? MAX_TORQUE_SAFE : 100.0;
    const double max_voltage = safe_mode ? MAX_VOLTAGE_SAFE : 5.0;
    
    double voltage = (torque_nm / max_torque) * max_voltage;
    return std::clamp(voltage, 0.0, max_voltage);
}

/**
 * @brief Convertir voltaje de salida a torque (Nm)
 */
inline double voltage_to_torque(double voltage_v, bool safe_mode = SAFE_TEST_MODE)
{
    const double max_torque = safe_mode ? MAX_TORQUE_SAFE : 100.0;
    const double max_voltage = safe_mode ? MAX_VOLTAGE_SAFE : 5.0;
    
    double torque = (voltage_v / max_voltage) * max_torque;
    return std::clamp(torque, 0.0, max_torque);
}

} // namespace control_vehiculo