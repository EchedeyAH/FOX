#pragma once
/**
 * motor_timeout_detector.hpp
 * Detector de timeout para motores M1-M4 basado en mensajes CAN/heartbeat.
 * 
 * Funcionalidad:
 *   - Cada mensaje válido de motor actualiza last_seen timestamp
 *   - Si now - last_seen > 2000ms → raise error CRITICO
 *   - Si motor vuelve a responder → clear error
 *   - Acción: TORQUE_0 o SAFE_STOP según política
 * 
 * IDs CAN que cuentan como heartbeat (definidos en can_protocol.hpp):
 *   - ID_MOTOR_1_RESP (0x181) .. ID_MOTOR_4_RESP (0x184)
 *   - Cualquier mensaje CCP response cuenta
 */

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>

#include "error_catalog.hpp"
#include "error_publisher.hpp"

namespace common {

// Timeout configurables
constexpr uint32_t MOTOR_TIMEOUT_MS = 2000;      // 2 segundos
constexpr uint32_t MOTOR_DEBOUNCE_MS = 100;       // Debounce para recuperación
constexpr double   DEFAULT_TORQUE_LIMIT = 0.0;    // 0 = parada total

// Estado de cada motor
struct MotorTimeoutState {
    uint64_t last_seen_ms = 0;       // Timestamp último mensaje válido
    bool timeout_active = false;      // Timeout actualmente activo
    uint64_t timeout_start_ms = 0;    // Cuándo empezó el timeout
    int reconnect_count = 0;          // Número de reconexiones
};

/**
 * MotorTimeoutDetector - Detecta timeout de 4 motores
 */
class MotorTimeoutDetector {
public:
    MotorTimeoutDetector();
    
    // Actualizar timestamp al recibir mensaje de un motor
    void on_motor_message(uint8_t motor_idx);
    
    // Verificar todos los motores (llamar desde watchdog)
    void check_all(time_t now_ms);
    
    // Obtener estado de un motor
    const MotorTimeoutState& get_state(uint8_t motor_idx) const;
    
    // Verificar si algún motor está en timeout
    bool any_timeout() const;
    
    // Obtener número de motores en timeout
    int timeout_count() const;
    
    // Obtener mask de motores en timeout (bit 0 = M1, etc)
    uint8_t get_timeout_mask() const;
    
    // Habilitar/deshabilitar detección
    void enable(bool en) { enabled_.store(en); }
    bool is_enabled() const { return enabled_.load(); }
    
    // Reset completo
    void reset();

private:
    void raise_timeout(uint8_t motor_idx, time_t now_ms);
    void clear_timeout(uint8_t motor_idx);
    
    std::array<MotorTimeoutState, 4> motors_;
    std::atomic<bool> enabled_;
};

// Implementación inline
inline MotorTimeoutDetector::MotorTimeoutDetector() : enabled_(true) {
    reset();
}

inline void MotorTimeoutDetector::reset() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    for (auto& m : motors_) {
        m.last_seen_ms = now;
        m.timeout_active = false;
        m.timeout_start_ms = 0;
        m.reconnect_count = 0;
    }
}

inline void MotorTimeoutDetector::on_motor_message(uint8_t motor_idx) {
    if (motor_idx >= 4 || !enabled_.load()) return;
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    auto& m = motors_[motor_idx];
    m.last_seen_ms = now;
    
    // Si había timeout activo, verificar debounce
    if (m.timeout_active) {
        if (now - m.timeout_start_ms >= MOTOR_DEBOUNCE_MS) {
            clear_timeout(motor_idx);
        }
    }
}

inline void MotorTimeoutDetector::check_all(time_t now_ms) {
    if (!enabled_.load()) return;
    
    for (uint8_t i = 0; i < 4; ++i) {
        auto& m = motors_[i];
        
        if (!m.timeout_active) {
            // Verificar si hay timeout
            if (now_ms - m.last_seen_ms > MOTOR_TIMEOUT_MS) {
                raise_timeout(i, now_ms);
            }
        }
    }
}

inline void MotorTimeoutDetector::raise_timeout(uint8_t motor_idx, time_t now_ms) {
    auto& m = motors_[motor_idx];
    m.timeout_active = true;
    m.timeout_start_ms = now_ms;
    
    // Determinar código de error según motor
    ecu::ErrorCode code;
    switch (motor_idx) {
        case 0: code = ecu::ErrorCode::M1_COM_TIMEOUT; break;
        case 1: code = ecu::ErrorCode::M2_COM_TIMEOUT; break;
        case 2: code = ecu::ErrorCode::M3_COM_TIMEOUT; break;
        case 3: code = ecu::ErrorCode::M4_COM_TIMEOUT; break;
        default: code = ecu::ErrorCode::ERR_INTERNAL;
    }
    
    // Crear evento de error
    ecu::ErrorEvent evt;
    evt.timestamp_ms = now_ms;
    evt.code = code;
    evt.level = ecu::ErrorLevel::CRITICO;
    evt.group = ecu::ErrorGroup::MOTOR;
    evt.status = ecu::ErrorStatus::ACTIVO;
    
    char origin[4] = "M1";
    origin[1] = '1' + motor_idx;
    evt.origin = origin;
    
    evt.description = "Motor timeout > 2s";
    evt.count = ++m.reconnect_count;
    
    // Publicar al HMI
    ecu::g_error_publisher.publish_event(evt);
    
    LOG_ERROR("MOTOR_TIMEOUT", std::string("Motor ") + (char)('1' + motor_idx) + " timeout (>2s)");
}

inline void MotorTimeoutDetector::clear_timeout(uint8_t motor_idx) {
    auto& m = motors_[motor_idx];
    m.timeout_active = false;
    
    // Determinar código de error según motor
    ecu::ErrorCode code;
    switch (motor_idx) {
        case 0: code = ecu::ErrorCode::M1_COM_TIMEOUT; break;
        case 1: code = ecu::ErrorCode::M2_COM_TIMEOUT; break;
        case 2: code = ecu::ErrorCode::M3_COM_TIMEOUT; break;
        case 3: code = ecu::ErrorCode::M4_COM_TIMEOUT; break;
        default: code = ecu::ErrorCode::ERR_INTERNAL;
    }
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Crear evento de resolución
    ecu::ErrorEvent evt;
    evt.timestamp_ms = now;
    evt.code = code;
    evt.level = ecu::ErrorLevel::INFORMATIVO;
    evt.group = ecu::ErrorGroup::MOTOR;
    evt.status = ecu::ErrorStatus::RESUELTO;
    
    char origin[4] = "M1";
    origin[1] = '1' + motor_idx;
    evt.origin = origin;
    
    evt.description = "Motor recovered";
    evt.count = m.reconnect_count;
    
    // Publicar al HMI
    ecu::g_error_publisher.publish_event(evt);
    
    LOG_INFO("MOTOR_TIMEOUT", std::string("Motor ") + (char)('1' + motor_idx) + " recovered");
}

inline const MotorTimeoutState& MotorTimeoutDetector::get_state(uint8_t motor_idx) const {
    return motors_[motor_idx];
}

inline bool MotorTimeoutDetector::any_timeout() const {
    for (const auto& m : motors_) {
        if (m.timeout_active) return true;
    }
    return false;
}

inline int MotorTimeoutDetector::timeout_count() const {
    int count = 0;
    for (const auto& m : motors_) {
        if (m.timeout_active) count++;
    }
    return count;
}

inline uint8_t MotorTimeoutDetector::get_timeout_mask() const {
    uint8_t mask = 0;
    for (int i = 0; i < 4; ++i) {
        if (motors_[i].timeout_active) {
            mask |= (1 << i);
        }
    }
    return mask;
}

// Instancia global
inline MotorTimeoutDetector& get_motor_timeout_detector() {
    static MotorTimeoutDetector detector;
    return detector;
}

} // namespace common
