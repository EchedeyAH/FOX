#include "error_system.hpp"
#include "logging.hpp"

#include <algorithm>
#include <sstream>

namespace common {

// ============================================================================
// IMPLEMENTACIÓN DEL ERROR MANAGER
// ============================================================================

ErrorManager::ErrorManager() {
    // Inicializar timestamps de heartbeat
    auto now = std::chrono::steady_clock::now();
    last_heartbeat_.fill(now);
    errors_.fill(ErrorEvent{});
}

ErrorEvent* ErrorManager::find_error(uint16_t code) {
    auto it = std::find_if(errors_.begin(), errors_.end(),
        [code](const ErrorEvent& e) { return e.code == code && e.active; });
    return (it != errors_.end()) ? &(*it) : nullptr;
}

const ErrorEvent* ErrorManager::find_error(uint16_t code) const {
    auto it = std::find_if(errors_.begin(), errors_.end(),
        [code](const ErrorEvent& e) { return e.code == code && e.active; });
    return (it != errors_.end()) ? &(*it) : nullptr;
}

ErrorLevel ErrorManager::deduce_level(uint16_t code) const {
    // Deducir nivel del código
    uint8_t category = (code >> 8) & 0x0F;
    
    switch (category) {
        case 0x1: return ErrorLevel::LEVE;   // COMUNICACION
        case 0x2: return ErrorLevel::GRAVE;  // HARDWARE
        case 0x3: return ErrorLevel::CRITICO; // SOFTWARE
        case 0x4: return ErrorLevel::LEVE;    // SENSOR
        case 0x5: return ErrorLevel::GRAVE;  // ACTUADOR
        case 0x6: return ErrorLevel::LEVE;   // POTENCIA
        case 0x7: return ErrorLevel::GRAVE;  // TEMPERATURA
        case 0x8: return ErrorLevel::GRAVE;  // TIMEOUT
        case 0x9: return ErrorLevel::LEVE;   // VALIDACION
        case 0xA: return ErrorLevel::CRITICO; // WATCHDOG
        default: return ErrorLevel::LEVE;
    }
}

void ErrorManager::log_error_event(const ErrorEvent& err, ErrorLevel new_level) {
    std::ostringstream msg;
    msg << "Error: 0x" << std::hex << err.code << std::dec 
        << " [" << error_level_str(new_level) << "]"
        << " " << subsystem_str(err.subsystem)
        << " count=" << err.count;
    LOG_ERROR("ErrorSystem", msg.str());
}

bool ErrorManager::raise_error(uint16_t code, ErrorLevel forced_level) {
    auto now = std::chrono::steady_clock::now();
    
    // Buscar error existente o slot libre
    ErrorEvent* event = find_error(code);
    if (!event) {
        // Buscar slot libre
        event = const_cast<ErrorEvent*>(
            std::find_if(errors_.begin(), errors_.end(),
                [](const ErrorEvent& e) { return !e.active; }));
    }
    
    if (!event) {
        LOG_WARN("ErrorSystem", "Error pool exhausted, cannot register: 0x" + 
                 std::to_string(code));
        return false;
    }
    
    // Determinar nivel
    ErrorLevel level = (forced_level != ErrorLevel::OK) ? forced_level : deduce_level(code);
    
    if (!event->active) {
        // Nuevo error
        event->code = code;
        event->level = level;
        event->subsystem = static_cast<Subsystem>((code >> 12) & 0x0F);
        event->active = true;
        event->first_occurrence = now;
        event->description = "Error 0x" + std::to_string(code);
    }
    
    // Actualizar contadores
    event->count++;
    event->last_occurrence = now;
    event->reset_count = 0;
    total_errors_++;
    
    // Log según nivel
    LOG_WARN("ErrorSystem", "Error raised: 0x" + std::to_string(code) + 
             " [" + error_level_str(level) + "] count=" + std::to_string(event->count));
    
    return true;
}

void ErrorManager::clear_error(uint16_t code) {
    ErrorEvent* event = const_cast<ErrorEvent*>(find_error(code));
    if (event) {
        event->active = false;
        event->count = 0;
        event->reset_count = 0;
        LOG_INFO("ErrorSystem", "Error cleared: 0x" + std::to_string(code));
    }
}

void ErrorManager::clear_all() {
    for (auto& event : errors_) {
        event.active = false;
        event.count = 0;
        event.reset_count = 0;
    }
    total_errors_ = 0;
    LOG_INFO("ErrorSystem", "All errors cleared");
}

ErrorLevel ErrorManager::get_system_level() const {
    ErrorLevel max_level = ErrorLevel::OK;
    for (const auto& event : errors_) {
        if (event.active && event.level > max_level) {
            max_level = event.level;
        }
    }
    return max_level;
}

void ErrorManager::get_system_status(ErrorLevel& level, uint16_t& active_errors, 
                                   uint32_t& total) const {
    level = get_system_level();
    active_errors = 0;
    for (const auto& event : errors_) {
        if (event.active) active_errors++;
    }
    total = total_errors_.load();
}

void ErrorManager::update_heartbeat(Subsystem subsystem) {
    auto idx = static_cast<size_t>(subsystem);
    if (idx < last_heartbeat_.size()) {
        last_heartbeat_[idx] = std::chrono::steady_clock::now();
        heartbeat_initialized_ = true;
    }
}

uint32_t ErrorManager::get_time_since_heartbeat(Subsystem subsystem) const {
    auto idx = static_cast<size_t>(subsystem);
    if (idx >= last_heartbeat_.size()) return 0xFFFFFFFF;
    
    auto now = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_heartbeat_[idx]).count();
    return static_cast<uint32_t>(diff);
}

bool ErrorManager::check_watchdogs() {
    bool has_critical = false;
    
    if (!heartbeat_initialized_) return false;
    
    auto now = std::chrono::steady_clock::now();
    
    // === Supervisor Watchdog ===
    uint32_t time_since_superv = get_time_since_heartbeat(Subsystem::SUPERVISOR);
    if (time_since_superv > config_.hb_timeout_ms) {
        raise_error(Errors::SUPERV_HB_TIMEOUT, ErrorLevel::CRITICO);
        has_critical = true;
        LOG_ERROR("Watchdog", "Supervisor heartbeat timeout: " + 
                  std::to_string(time_since_superv) + "ms");
    }
    
    // === BMS Watchdog ===
    uint32_t time_since_bms = get_time_since_heartbeat(Subsystem::BMS);
    if (time_since_bms > config_.bms_timeout_ms) {
        raise_error(Errors::BMS_COM, ErrorLevel::GRAVE);
        LOG_ERROR("Watchdog", "BMS heartbeat timeout: " + 
                  std::to_string(time_since_bms) + "ms");
    }
    
    // === Motor Watchdogs (M1-M4) ===
    Subsystem motors[] = {Subsystem::MOTOR1, Subsystem::MOTOR2, 
                         Subsystem::MOTOR3, Subsystem::MOTOR4};
    const char* motor_names[] = {"M1", "M2", "M3", "M4"};
    
    for (int i = 0; i < 4; i++) {
        uint32_t time_since_motor = get_time_since_heartbeat(motors[i]);
        if (time_since_motor > config_.motor_hb_timeout_ms) {
            uint16_t error_code = Errors::MOTOR_COM_BASE + (i << 8);
            raise_error(error_code, ErrorLevel::CRITICO);
            has_critical = true;
            LOG_ERROR("Watchdog", "Motor " + std::string(motor_names[i]) + 
                      " heartbeat timeout: " + std::to_string(time_since_motor) + "ms");
        }
    }
    
    return has_critical;
}

std::string ErrorManager::dump_active_errors() const {
    std::ostringstream out;
    out << "=== Active Errors ===" << std::endl;
    
    for (const auto& event : errors_) {
        if (event.active) {
            out << "0x" << std::hex << event.code << std::dec << " "
                << "[" << error_level_str(event.level) << "] "
                << subsystem_str(event.subsystem) << " "
                << "count=" << event.count << std::endl;
        }
    }
    
    return out.str();
}

uint32_t ErrorManager::get_error_count(uint16_t code) const {
    const ErrorEvent* event = find_error(code);
    return event ? event->count : 0;
}

} // namespace common
