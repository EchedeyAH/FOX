#pragma once
/**
 * voltage_protection.hpp
 * Protección de voltaje de batería (Vbat) con umbrales y acciones.
 * 
 * Funcionalidad:
 *   - Monitoreo de Vpack (voltaje del pack de batería)
 *   - Detección de voltaje bajo/alto con histéresis
 *   - Limitación de potencia (LIMP_MODE) o parada (SAFE_STOP)
 *   - Integración con ErrorManager y HMI
 * 
 * Umbrales (pueden configurarse) para pack nominal ~61V:
 *   - Vbat bajo crítico: < 50V → CRITICO → SAFE_STOP
 *   - Vbat bajo: < 55V → GRAVE → LIMP_MODE
 *   - Vbat alto: > 67V → GRAVE → LIMP_MODE
 *   - Vbat crítico alto: > 70V → CRITICO → SAFE_STOP
 */

#include <atomic>
#include <chrono>
#include <cstdint>

#include "logging.hpp"
#include "error_catalog.hpp"
#include "error_publisher.hpp"

namespace common {

// Umbrales de voltaje (en milivoltios)
constexpr int32_t VBAT_CRITICAL_LOW_MV   = 50000;  // < 50V  → CRITICO
constexpr int32_t VBAT_LOW_MV            = 55000;  // < 55V  → GRAVE
constexpr int32_t VBAT_HIGH_MV           = 67000;  // > 67V  → GRAVE
constexpr int32_t VBAT_CRITICAL_HIGH_MV  = 70000;  // > 70V  → CRITICO

// Histéresis para recuperación (mV)
constexpr int32_t VBAT_HYSTERESIS_MV    = 5000;   // 5V de histéresis

// Tiempos
constexpr uint32_t VOLTAGE_DEBOUNCE_MS   = 500;    // Debounce para cambios

/**
 * Estado de protección de voltaje
 */
enum class VoltageState {
    NORMAL = 0,
    LOW_WARNING = 1,      // Vbat cerca de bajo
    LOW_CRITICAL = 2,     // Vbat demasiado bajo
    HIGH_WARNING = 3,     // Vbat cerca de alto  
    HIGH_CRITICAL = 4,     // Vbat demasiado alto
    DISABLED = 5
};

/**
 * Resultado de verificación de voltaje
 */
struct VoltageCheckResult {
    VoltageState state;
    bool limit_power;       // true = aplicar limitación de potencia
    bool safe_stop;         // true = parada segura inmediata
    double power_factor;    // Factor de limitación (0.0-1.0)
};

/**
 * VoltageProtection - Monitor y protección de voltaje batería
 */
class VoltageProtection {
public:
    VoltageProtection();
    
    // Configurar umbrales (opcional)
    void set_thresholds(int32_t critical_low, int32_t low, int32_t high, int32_t critical_high);
    
    // Verificar voltaje actual (llamar desde thread de control)
    VoltageCheckResult check_voltage(int32_t voltage_mv);
    
    // Obtener estado actual
    VoltageState get_state() const { return state_.load(); }
    
    // Obtener factor de limitación actual
    double get_power_factor() const { return power_factor_.load(); }
    
    // Habilitar/deshabilitar
    void enable(bool en) { enabled_.store(en); }
    bool is_enabled() const { return enabled_.load(); }
    
    // Reset
    void reset();

private:
    void update_state(VoltageState new_state, int32_t voltage_mv);
    void apply_limit(double factor, const char* reason);
    void clear_limit();
    
    // Estado
    std::atomic<VoltageState> state_;
    std::atomic<double> power_factor_;    // 1.0 = sin límite
    std::atomic<bool> enabled_;
    std::atomic<bool> limiting_;
    
    // Umbrales
    int32_t threshold_critical_low_;
    int32_t threshold_low_;
    int32_t threshold_high_;
    int32_t threshold_critical_high_;
    
    // Timestamps
    uint64_t last_warning_time_ms_;
    uint64_t last_critical_time_ms_;
};

// Implementación inline
inline VoltageProtection::VoltageProtection() 
    : state_(VoltageState::NORMAL)
    , power_factor_(1.0)
    , enabled_(true)
    , limiting_(false)
    , threshold_critical_low_(VBAT_CRITICAL_LOW_MV)
    , threshold_low_(VBAT_LOW_MV)
    , threshold_high_(VBAT_HIGH_MV)
    , threshold_critical_high_(VBAT_CRITICAL_HIGH_MV)
    , last_warning_time_ms_(0)
    , last_critical_time_ms_(0)
{}

inline void VoltageProtection::set_thresholds(int32_t critical_low, int32_t low, 
                                               int32_t high, int32_t critical_high) {
    threshold_critical_low_ = critical_low;
    threshold_low_ = low;
    threshold_high_ = high;
    threshold_critical_high_ = critical_high;
}

inline VoltageCheckResult VoltageProtection::check_voltage(int32_t voltage_mv) {
    VoltageCheckResult result{};
    result.state = VoltageState::NORMAL;
    result.limit_power = false;
    result.safe_stop = false;
    result.power_factor = 1.0;
    
    if (!enabled_.load()) {
        return result;
    }
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Verificar voltaje contra umbrales
    if (voltage_mv < threshold_critical_low_) {
        // Crítico bajo - SAFE_STOP
        result.state = VoltageState::LOW_CRITICAL;
        result.safe_stop = true;
        result.power_factor = 0.0;
        
        if (state_.load() != VoltageState::LOW_CRITICAL) {
            update_state(VoltageState::LOW_CRITICAL, voltage_mv);
        }
    }
    else if (voltage_mv < threshold_low_) {
        // Bajo - LIMP_MODE
        result.state = VoltageState::LOW_WARNING;
        result.limit_power = true;
        result.power_factor = 0.3; // 30% potencia
        
        if (state_.load() != VoltageState::LOW_WARNING) {
            update_state(VoltageState::LOW_WARNING, voltage_mv);
        }
    }
    else if (voltage_mv > threshold_critical_high_) {
        // Crítico alto - SAFE_STOP
        result.state = VoltageState::HIGH_CRITICAL;
        result.safe_stop = true;
        result.power_factor = 0.0;
        
        if (state_.load() != VoltageState::HIGH_CRITICAL) {
            update_state(VoltageState::HIGH_CRITICAL, voltage_mv);
        }
    }
    else if (voltage_mv > threshold_high_) {
        // Alto - LIMP_MODE
        result.state = VoltageState::HIGH_WARNING;
        result.limit_power = true;
        result.power_factor = 0.5; // 50% potencia
        
        if (state_.load() != VoltageState::HIGH_WARNING) {
            update_state(VoltageState::HIGH_WARNING, voltage_mv);
        }
    }
    else {
        // Normal - verificar recuperación
        VoltageState old = state_.load();
        if (old != VoltageState::NORMAL) {
            // Verificar histéresis para recuperación
            bool can_recover = false;
            if (old == VoltageState::LOW_WARNING || old == VoltageState::LOW_CRITICAL) {
                can_recover = (voltage_mv > (threshold_low_ + VBAT_HYSTERESIS_MV));
            }
            else if (old == VoltageState::HIGH_WARNING || old == VoltageState::HIGH_CRITICAL) {
                can_recover = (voltage_mv < (threshold_high_ - VBAT_HYSTERESIS_MV));
            }
            
            if (can_recover) {
                update_state(VoltageState::NORMAL, voltage_mv);
                clear_limit();
            }
        }
        
        result.state = state_.load();
        result.power_factor = power_factor_.load();
    }
    
    return result;
}

inline void VoltageProtection::update_state(VoltageState new_state, int32_t voltage_mv) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Determinar código de error
    ecu::ErrorCode code;
    ecu::ErrorLevel level;
    const char* desc;
    bool is_critical = false;
    
    switch (new_state) {
        case VoltageState::LOW_CRITICAL:
            code = ecu::ErrorCode::BMS_VOLT_LOW;
            level = ecu::ErrorLevel::CRITICO;
            desc = "Vbat crítico bajo";
            is_critical = true;
            last_critical_time_ms_ = now;
            break;
            
        case VoltageState::LOW_WARNING:
            code = ecu::ErrorCode::BMS_VOLT_LOW;
            level = ecu::ErrorLevel::GRAVE;
            desc = "Vbat bajo";
            last_warning_time_ms_ = now;
            break;
            
        case VoltageState::HIGH_CRITICAL:
            code = ecu::ErrorCode::BMS_VOLT_HIGH;
            level = ecu::ErrorLevel::CRITICO;
            desc = "Vbat crítico alto";
            is_critical = true;
            last_critical_time_ms_ = now;
            break;
            
        case VoltageState::HIGH_WARNING:
            code = ecu::ErrorCode::BMS_VOLT_HIGH;
            level = ecu::ErrorLevel::GRAVE;
            desc = "Vbat alto";
            last_warning_time_ms_ = now;
            break;
            
        default:
            // Recuperación - no generar evento
            state_.store(new_state);
            return;
    }
    
    // Publicar evento
    ecu::ErrorEvent evt;
    evt.timestamp_ms = now;
    evt.code = code;
    evt.level = level;
    evt.group = ecu::ErrorGroup::BMS;
    evt.status = ecu::ErrorStatus::ACTIVO;
    evt.origin = "VBAT";
    evt.description = desc;
    evt.count = 1;
    
    ecu::g_error_publisher.publish_event(evt);
    
    // Aplicar acción según nivel
    if (is_critical) {
        // SAFE_STOP
        power_factor_.store(0.0);
        limiting_.store(true);
    }
    else if (new_state == VoltageState::LOW_WARNING) {
        apply_limit(0.3, "Vbat bajo");
    }
    else if (new_state == VoltageState::HIGH_WARNING) {
        apply_limit(0.5, "Vbat alto");
    }
    
    state_.store(new_state);
    
    LOG_WARN("VBAT", std::string(desc) + ": " + std::to_string(voltage_mv/1000) + "V");
}

inline void VoltageProtection::apply_limit(double factor, const char* reason) {
    power_factor_.store(factor);
    limiting_.store(true);
    LOG_WARN("VBAT", std::string(reason) + " -> limitando potencia a " + std::to_string(int(factor*100)) + "%");
}

inline void VoltageProtection::clear_limit() {
    power_factor_.store(1.0);
    limiting_.store(false);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Publicar resolución
    ecu::ErrorEvent evt;
    evt.timestamp_ms = now;
    evt.code = ecu::ErrorCode::BMS_VOLT_LOW; // O BMS_VOLT_HIGH según corresponda
    evt.level = ecu::ErrorLevel::INFORMATIVO;
    evt.group = ecu::ErrorGroup::BMS;
    evt.status = ecu::ErrorStatus::RESUELTO;
    evt.origin = "VBAT";
    evt.description = "Vbat recuperado";
    evt.count = 1;
    
    ecu::g_error_publisher.publish_event(evt);
    
    LOG_INFO("VBAT", "Voltaje恢复正常 -> potencia sin límites");
}

inline void VoltageProtection::reset() {
    state_.store(VoltageState::NORMAL);
    power_factor_.store(1.0);
    limiting_.store(false);
}

// Instancia global
inline VoltageProtection& get_voltage_protection() {
    static VoltageProtection protection;
    return protection;
}

} // namespace common
