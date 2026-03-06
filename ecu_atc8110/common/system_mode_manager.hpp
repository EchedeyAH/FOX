#pragma once
/**
 * system_mode_manager.hpp
 * Gestor de modos del sistema con transiciones y políticas.
 * 
 * Máquina de estados de modos:
 * 
 *     OK ----[error GRAVE]----> LIMP_MODE
 *     |                              |
 *     |                              |
 *     +<----[recuperación]----------+
 *     |
 *     +----[error CRITICO]--------> SAFE_STOP
 *     |
 *     +<----[reset]-----------------+
 * 
 * Políticas:
 *   - OK -> LIMP_MODE: errores GRAVE persistentes (temp, SOC bajo, Vbat, etc)
 *   - OK/LIMP -> SAFE_STOP: errores CRITICOS (timeout motor, BMS perdido, etc)
 *   - LIMP -> OK: condiciones normales + histéresis + tiempo estable
 *   - SAFE_STOP -> OK: solo con reset explícito
 * 
 * Acciones en cada modo:
 *   - OK: potencia completa, sin restricciones
 *   - LIMP_MODE: factor_potencia < 1.0, rampas suaves
 *   - SAFE_STOP: torque = 0, motor deshabilitado
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>

#include "error_catalog.hpp"
#include "error_publisher.hpp"

namespace common {

// Tiempos de transición
constexpr uint32_t LIMP_ENTRY_DELAY_MS = 1000;    // Tiempo antes de entrar en LIMP
constexpr uint32_t LIMP_EXIT_DELAY_MS = 3000;     // Tiempo estable antes de salir
constexpr uint32_t CRITICAL_HYSTERESIS_MS = 500;  // Histéresis para críticos

// Factores de potencia
constexpr double LIMP_TEMP_FACTOR = 0.5;      // 50% por temperatura
constexpr double LIMP_SOC_FACTOR = 0.3;       // 30% por SOC bajo
constexpr double LIMP_VBAT_FACTOR = 0.3;     // 30% por Vbat bajo
constexpr double LIMP_DEFAULT_FACTOR = 0.5;   // 50% por otros

// Límites
constexpr int MAX_LIMP_TRIGGERS = 3;         // Veces en LIMP antes de SAFE_STOP

/**
 * Modos del sistema
 */
enum class SystemMode {
    OK = 0,          // Normal, potencia completa
    LIMP_MODE = 1,   // Degradado, potencia limitada
    SAFE_STOP = 2    // Parada segura, sin potencia
};

/**
 * Razón de transición a LIMP_MODE
 */
enum class LimpReason {
    NONE = 0,
    TEMPERATURE = 1,    // Temperatura motor alta
    SOC_LOW = 2,        // SOC batería bajo
    VBAT_LOW = 3,      // Voltaje batería bajo
    SCHED_OVERRUN = 4, // Scheduler overruns
    OTHER = 5
};

/**
 * Estado del gestor de modos
 */
struct ModeManagerState {
    SystemMode mode;
    LimpReason limp_reason;
    double power_factor;
    uint64_t mode_start_ms;
    uint64_t last_normal_ms;
    int limp_trigger_count;
    bool torque_zero_commanded;
};

/**
 * SystemModeManager - Gestor centralizado de modos
 */
class SystemModeManager {
public:
    SystemModeManager();
    
    // ─────────────────────────────────────────────────────────────────────
    // APIs de entrada de errores (llamar desde detectores)
    // ─────────────────────────────────────────────────────────────────────
    
    // Notificar error GRAVE (puede causar LIMP_MODE)
    void notify_grave_error(LimpReason reason);
    
    // Notificar error CRITICO (causa SAFE_STOP inmediato)
    void notify_critical_error(ecu::ErrorCode code);
    
    // Notificar recuperación de error
    void notify_error_resolved();
    
    // ─────────────────────────────────────────────────────────────────────
    // APIs de consulta (llamar desde thread de control)
    // ─────────────────────────────────────────────────────────────────────
    
    // Obtener modo actual
    SystemMode get_mode() const { return mode_.load(); }
    
    // Obtener factor de potencia actual
    double get_power_factor() const { return power_factor_.load(); }
    
    // Verificar si torque debe ser 0
    bool is_torque_zero() const { return torque_zero_.load(); }
    
    // Obtener estado completo
    ModeManagerState get_state() const;
    
    // ─────────────────────────────────────────────────────────────────────
    // APIs de control
    // ─────────────────────────────────────────────────────────────────────
    
    // Forzar modo (para testing)
    void force_mode(SystemMode mode);
    
    // Reset completo (para salir de SAFE_STOP)
    void reset();
    
    // Procesar ciclo (llamar periódicamente para actualizar timers)
    void process_cycle();

private:
    void transition_to_limp(LimpReason reason);
    void transition_to_safe_stop(ecu::ErrorCode cause);
    void transition_to_ok();
    bool can_exit_limp() const;
    void apply_power_limit(double factor);
    void clear_power_limit();
    void publish_mode_event(SystemMode old_mode, SystemMode new_mode);

    // Estado
    std::atomic<SystemMode> mode_;
    std::atomic<LimpReason> limp_reason_;
    std::atomic<double> power_factor_;
    std::atomic<bool> torque_zero_;
    
    // Timers
    uint64_t mode_start_ms_;
    uint64_t last_normal_ms_;
    int limp_trigger_count_;
    
    // Tracking de razones
    std::array<bool, 6> active_grave_reasons_;
};

// Implementación inline
inline SystemModeManager::SystemModeManager()
    : mode_(SystemMode::OK)
    , limp_reason_(LimpReason::NONE)
    , power_factor_(1.0)
    , torque_zero_(false)
    , mode_start_ms_(0)
    , last_normal_ms_(0)
    , limp_trigger_count_(0)
{
    active_grave_reasons_.fill(false);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    mode_start_ms_ = now;
    last_normal_ms_ = now;
}

inline void SystemModeManager::notify_grave_error(LimpReason reason) {
    if (mode_.load() == SystemMode::SAFE_STOP) return; // Ya en stop
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Marcar razón como activa
    if (reason >= LimpReason::NONE && reason < LimpReason::OTHER) {
        active_grave_reasons_[static_cast<int>(reason)] = true;
    }
    
    // Si ya en LIMP, solo actualizar razón si es más severa
    if (mode_.load() == SystemMode::LIMP_MODE) {
        // Ya en limp, puede que necesitemos ajustar factor
        // Por ahora, mantener el más restrictivo
        return;
    }
    
    // Verificar delay antes de entrar en LIMP
    if (now - mode_start_ms_ >= LIMP_ENTRY_DELAY_MS) {
        transition_to_limp(reason);
    }
}

inline void SystemModeManager::notify_critical_error(ecu::ErrorCode code) {
    // Error crítico causa SAFE_STOP inmediato
    transition_to_safe_stop(code);
}

inline void SystemModeManager::notify_error_resolved() {
    // Marcar todas las razones como resueltas
    active_grave_reasons_.fill(false);
    
    // Si estamos en LIMP, verificar si podemos salir
    if (mode_.load() == SystemMode::LIMP_MODE) {
        if (can_exit_limp()) {
            transition_to_ok();
        }
    }
}

inline ModeManagerState SystemModeManager::get_state() const {
    ModeManagerState state;
    state.mode = mode_.load();
    state.limp_reason = limp_reason_.load();
    state.power_factor = power_factor_.load();
    state.mode_start_ms = mode_start_ms_;
    state.last_normal_ms = last_normal_ms_;
    state.limp_trigger_count = limp_trigger_count_;
    state.torque_zero_commanded = torque_zero_.load();
    return state;
}

inline void SystemModeManager::force_mode(SystemMode mode) {
    auto old_mode = mode_.load();
    mode_.store(mode);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    mode_start_ms_ = now;
    
    if (mode == SystemMode::SAFE_STOP) {
        torque_zero_.store(true);
        power_factor_.store(0.0);
    }
    else if (mode == SystemMode::LIMP_MODE) {
        torque_zero_.store(false);
        apply_power_limit(LIMP_DEFAULT_FACTOR);
    }
    else {
        torque_zero_.store(false);
        clear_power_limit();
    }
    
    publish_mode_event(old_mode, mode);
}

inline void SystemModeManager::reset() {
    auto old_mode = mode_.load();
    
    mode_.store(SystemMode::OK);
    limp_reason_.store(LimpReason::NONE);
    power_factor_.store(1.0);
    torque_zero_.store(false);
    limp_trigger_count_ = 0;
    active_grave_reasons_.fill(false);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    mode_start_ms_ = now;
    last_normal_ms_ = now;
    
    publish_mode_event(old_mode, SystemMode::OK);
    
    LOG_INFO("MODE_MGR", "Sistema reseteado -> OK");
}

inline void SystemModeManager::process_cycle() {
    // Por ahora, no hay procesamiento automático adicional
    // Las transiciones se hacen por eventos
}

inline void SystemModeManager::transition_to_limp(LimpReason reason) {
    SystemMode old_mode = mode_.load();
    if (old_mode == SystemMode::LIMP_MODE) return;
    
    limp_reason_.store(reason);
    limp_trigger_count_++;
    
    // Determinar factor según razón
    double factor = LIMP_DEFAULT_FACTOR;
    switch (reason) {
        case LimpReason::TEMPERATURE: factor = LIMP_TEMP_FACTOR; break;
        case LimpReason::SOC_LOW: factor = LIMP_SOC_FACTOR; break;
        case LimpReason::VBAT_LOW: factor = LIMP_VBAT_FACTOR; break;
        default: factor = LIMP_DEFAULT_FACTOR;
    }
    
    mode_.store(SystemMode::LIMP_MODE);
    apply_power_limit(factor);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    mode_start_ms_ = now;
    
    // Si demasiados triggers, ir a SAFE_STOP
    if (limp_trigger_count_ >= MAX_LIMP_TRIGGERS) {
        LOG_WARN("MODE_MGR", "Demasiados LIMP triggers -> SAFE_STOP");
        transition_to_safe_stop(ecu::ErrorCode::ERR_INTERNAL);
        return;
    }
    
    publish_mode_event(old_mode, SystemMode::LIMP_MODE);
    
    LOG_WARN("MODE_MGR", "-> LIMP_MODE reason=" + std::to_string(static_cast<int>(reason)));
}

inline void SystemModeManager::transition_to_safe_stop(ecu::ErrorCode cause) {
    SystemMode old_mode = mode_.load();
    if (old_mode == SystemMode::SAFE_STOP) return;
    
    mode_.store(SystemMode::SAFE_STOP);
    torque_zero_.store(true);
    power_factor_.store(0.0);
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    mode_start_ms_ = now;
    
    publish_mode_event(old_mode, SystemMode::SAFE_STOP);
    
    // Publicar error crítico
    ecu::ErrorEvent evt;
    evt.timestamp_ms = now;
    evt.code = cause;
    evt.level = ecu::ErrorLevel::CRITICO;
    evt.group = ecu::ErrorGroup::ERROR;
    evt.status = ecu::ErrorStatus::ACTIVO;
    evt.origin = "MODE_MGR";
    evt.description = "SAFE_STOP activated";
    evt.count = 1;
    
    ecu::g_error_publisher.publish_event(evt);
    
    LOG_ERROR("MODE_MGR", "-> SAFE_STOP cause=0x" + std::to_string(static_cast<uint16_t>(cause)));
}

inline void SystemModeManager::transition_to_ok() {
    SystemMode old_mode = mode_.load();
    if (old_mode == SystemMode::OK) return;
    
    mode_.store(SystemMode::OK);
    limp_reason_.store(LimpReason::NONE);
    clear_power_limit();
    
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    mode_start_ms_ = now;
    last_normal_ms_ = now;
    
    publish_mode_event(old_mode, SystemMode::OK);
    
    LOG_INFO("MODE_MGR", "-> OK (recuperado)");
}

inline bool SystemModeManager::can_exit_limp() const {
    // Verificar que no hay razones activas
    for (bool active : active_grave_reasons_) {
        if (active) return false;
    }
    
    // Verificar tiempo mínimo en LIMP
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (now - mode_start_ms_ < LIMP_EXIT_DELAY_MS) {
        return false;
    }
    
    return true;
}

inline void SystemModeManager::apply_power_limit(double factor) {
    power_factor_.store(factor);
    LOG_INFO("MODE_MGR", "Potencia limitada al " + std::to_string(int(factor*100)) + "%");
}

inline void SystemModeManager::clear_power_limit() {
    power_factor_.store(1.0);
}

inline void SystemModeManager::publish_mode_event(SystemMode old_mode, SystemMode new_mode) {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    const char* old_str = (old_mode == SystemMode::OK) ? "OK" : 
                          (old_mode == SystemMode::LIMP_MODE) ? "LIMP" : "STOP";
    const char* new_str = (new_mode == SystemMode::OK) ? "OK" : 
                          (new_mode == SystemMode::LIMP_MODE) ? "LIMP" : "STOP";
    
    ecu::ErrorEvent evt;
    evt.timestamp_ms = now;
    evt.code = ecu::ErrorCode::CTRL_FSM_ERR;
    evt.level = (new_mode == SystemMode::SAFE_STOP) ? ecu::ErrorLevel::CRITICO :
                (new_mode == SystemMode::LIMP_MODE) ? ecu::ErrorLevel::GRAVE : 
                ecu::ErrorLevel::INFORMATIVO;
    evt.group = ecu::ErrorGroup::CONTROL;
    evt.status = ecu::ErrorStatus::ACTIVO;
    evt.origin = "MODE_MGR";
    evt.description = std::string("Mode: ") + old_str + " -> " + new_str;
    evt.count = 1;
    
    ecu::g_error_publisher.publish_event(evt);
}

// Instancia global
inline SystemModeManager& get_system_mode_manager() {
    static SystemModeManager mgr;
    return mgr;
}

} // namespace common
