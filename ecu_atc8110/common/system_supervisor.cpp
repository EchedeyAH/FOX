#include "system_supervisor.hpp"

namespace common {

// ============================================================================
// IMPLEMENTACIÓN DEL SYSTEM SUPERVISOR
// ============================================================================

SystemSupervisor::SystemSupervisor() {
    error_manager_ = std::make_unique<ErrorManager>();
    can_validator_ = std::make_unique<CanValidator>();
    power_manager_ = std::make_unique<PowerManager>();
}

bool SystemSupervisor::init() {
    LOG_INFO("SystemSupervisor", "Inicializando supervisor del sistema...");
    
    // Configurar timeouts por defecto
    ErrorConfig err_config;
    err_config.hb_timeout_ms = 500;
    err_config.motor_hb_timeout_ms = 2000;
    err_config.bms_timeout_ms = 1000;
    error_manager_->set_config(err_config);
    
    CanValidatorConfig can_config;
    can_config.motor_response_timeout_ms = 2000;
    can_config.bms_response_timeout_ms = 1000;
    can_config.supervisor_response_timeout_ms = 500;
    can_validator_->set_config(can_config);
    
    PowerManagerConfig pwr_config;
    pwr_config.max_power_w = 56000;
    pwr_config.max_battery_power_w = 18000;
    power_manager_->set_config(pwr_config);
    
    initialized_ = true;
    LOG_INFO("SystemSupervisor", "Supervisor inicializado correctamente");
    return true;
}

void SystemSupervisor::update(const SystemSnapshot& snapshot) {
    if (!initialized_) return;
    
    // 1. Actualizar errores desde snapshot
    update_errors_from_snapshot(snapshot);
    
    // 2. Actualizar límites de potencia
    update_power_limits(snapshot);
}

void SystemSupervisor::update_errors_from_snapshot(const SystemSnapshot& snapshot) {
    // Verificar estado de BMS
    if (!snapshot.battery.communication_ok) {
        error_manager_->raise_error(Errors::BMS_COM, ErrorLevel::GRAVE);
    }
    
    // Verificar alarmas BMS
    if (snapshot.battery.alarm_level >= 2) {  // ALARMA o superior
        error_manager_->raise_error(Errors::BMS_SYS_ERR, 
            snapshot.battery.alarm_level >= 3 ? ErrorLevel::CRITICO : ErrorLevel::GRAVE);
    }
    
    // Verificar motores
    for (size_t i = 0; i < snapshot.motors.size(); i++) {
        const auto& motor = snapshot.motors[i];
        
        // Check communication
        if (!motor.enabled && current_level_ > ErrorLevel::OK) {
            uint16_t motor_err = Errors::MOTOR_COM_BASE + (i << 8);
            error_manager_->raise_error(motor_err, ErrorLevel::CRITICO);
        }
        
        // Check temperatures
        if (motor.motor_temp_c > 80) {
            error_manager_->raise_error(Errors::MOTOR_TEMP_HIGH + (i << 8), ErrorLevel::CRITICO);
        } else if (motor.motor_temp_c > 60) {
            error_manager_->raise_error(Errors::MOTOR_TEMP_HIGH + (i << 8), ErrorLevel::LEVE);
        }
    }
    
    // Actualizar nivel de error
    current_level_ = error_manager_->get_system_level();
    
    // Verificar si necesitamos modo seguro
    if (current_level_ >= ErrorLevel::CRITICO) {
        handle_critical_error();
    }
}

void SystemSupervisor::update_power_limits(const SystemSnapshot& snapshot) {
    // Extraer datos para gestión de potencia
    double accelerator = snapshot.vehicle.accelerator;
    double brake = snapshot.vehicle.brake;
    double soc = snapshot.battery.state_of_charge;
    uint32_t vbat = static_cast<uint32_t>(snapshot.battery.pack_voltage_mv);
    int32_t ibat = static_cast<int32_t>(snapshot.battery.pack_current_ma);
    
    // Temperaturas de motores
    std::array<double, 4> motor_temps;
    for (size_t i = 0; i < 4; i++) {
        motor_temps[i] = snapshot.motors[i].motor_temp_c;
    }
    
    // Calcular potencia
    power_manager_->calculate_power(accelerator, brake, soc, vbat, ibat, motor_temps);
}

void SystemSupervisor::check_watchdogs() {
    if (!initialized_) return;
    
    // Verificar watchdogs de comunicación
    bool has_critical = can_validator_->check_communication_health();
    
    // También verificar watchdogs del manager de errores
    has_critical = error_manager_->check_watchdogs() || has_critical;
    
    if (has_critical) {
        current_level_ = ErrorLevel::CRITICO;
        handle_critical_error();
    }
}

bool SystemSupervisor::check_system_health() {
    if (!initialized_) return false;
    
    // Actualizar watchdogs
    check_watchdogs();
    
    // Verificar nivel de error
    current_level_ = error_manager_->get_system_level();
    
    return current_level_ >= ErrorLevel::CRITICO;
}

void SystemSupervisor::handle_critical_error() {
    if (safe_mode_) return;
    
    LOG_ERROR("SystemSupervisor", "ERROR CRÍTICO - Entrando en modo seguro");
    safe_mode_ = true;
}

double SystemSupervisor::get_power_limit_factor() const {
    if (!initialized_) return 1.0;
    return power_manager_->get_power_limit_factor();
}

ErrorLevel SystemSupervisor::get_error_level() const {
    return current_level_;
}

bool SystemSupervisor::should_enter_safe_mode() const {
    return safe_mode_ || current_level_ >= ErrorLevel::CRITICO;
}

double SystemSupervisor::get_max_allowed_torque() const {
    if (safe_mode_ || current_level_ >= ErrorLevel::CRITICO) {
        return max_torque_emergency_;
    } else if (current_level_ >= ErrorLevel::GRAVE) {
        return max_torque_degraded_;
    }
    return max_torque_normal_;
}

void SystemSupervisor::register_heartbeat(Subsystem subsystem) {
    if (initialized_) {
        error_manager_->update_heartbeat(subsystem);
    }
}

void SystemSupervisor::register_can_message(uint8_t peer_id) {
    if (initialized_) {
        can_validator_->register_rx(peer_id);
    }
}

void SystemSupervisor::set_error_config(const ErrorConfig& config) {
    if (error_manager_) {
        error_manager_->set_config(config);
    }
}

void SystemSupervisor::set_can_config(const CanValidatorConfig& config) {
    if (can_validator_) {
        can_validator_->set_config(config);
    }
}

void SystemSupervisor::set_power_config(const PowerManagerConfig& config) {
    if (power_manager_) {
        power_manager_->set_config(config);
    }
}

std::string SystemSupervisor::dump_status() const {
    std::ostringstream out;
    out << "=== System Supervisor Status ===" << std::endl;
    out << "Initialized: " << (initialized_ ? "YES" : "NO") << std::endl;
    out << "Safe Mode: " << (safe_mode_ ? "YES" : "NO") << std::endl;
    out << "Error Level: " << error_level_str(current_level_) << std::endl;
    out << "Max Torque: " << get_max_allowed_torque() << " Nm" << std::endl;
    out << "Power Limit: " << get_power_limit_factor() << std::endl;
    out << std::endl;
    
    if (error_manager_) {
        out << error_manager_->dump_active_errors();
    }
    
    out << std::endl;
    
    if (can_validator_) {
        out << can_validator_->dump_status();
    }
    
    out << std::endl;
    
    if (power_manager_) {
        out << power_manager_->dump_status();
    }
    
    return out.str();
}

} // namespace common
