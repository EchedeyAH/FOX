#include "power_manager.hpp"
#include "logging.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace common {

// ============================================================================
// IMPLEMENTACIÓN DEL POWER MANAGER
// ============================================================================

PowerManager::PowerManager() {
    reset();
}

void PowerManager::reset() {
    state_ = PowerState{};
    last_result_ = PowerResult::OK;
    power_limit_factor_ = 1.0;
    previous_demanded_power_ = 0.0;
    last_ramp_time_ = std::chrono::steady_clock::now();
    
    // Inicializar array de potencia por motor
    state_.motor_power_w.fill(0.0);
    
    LOG_INFO("PowerManager", "Initialized");
}

double PowerManager::calculate_soc_limit(double soc) {
    if (soc <= config_.soc_critical_limit) {
        // Modo crítico: potencia muy limitada
        state_.limited_by_soc = true;
        return 0.1; // 10% de potencia
    } else if (soc <= config_.soc_min_limit) {
        state_.limited_by_soc = true;
        return 0.3; // 30% de potencia
    } else if (soc <= config_.soc_normal_limit) {
        state_.limited_by_soc = true;
        return 0.6; // 60% de potencia
    } else if (soc >= config_.soc_full_limit) {
        // Batería llena: limitar carga
        state_.limited_by_soc = true;
        return 0.9;
    }
    
    state_.limited_by_soc = false;
    return 1.0;
}

double PowerManager::calculate_temperature_limit(const std::array<double, 4>& motor_temps) {
    double min_factor = 1.0;
    bool limited = false;
    
    for (size_t i = 0; i < motor_temps.size(); i++) {
        double temp = motor_temps[i];
        
        if (temp >= config_.temp_motor_max) {
            min_factor = std::min(min_factor, 0.2);
            limited = true;
        } else if (temp >= config_.temp_motor_limit) {
            double factor = 1.0 - ((temp - config_.temp_motor_limit) / 
                                   (config_.temp_motor_max - config_.temp_motor_limit)) * 0.5;
            min_factor = std::min(min_factor, factor);
            limited = true;
        }
    }
    
    state_.limited_by_temperature = limited;
    return min_factor;
}

double PowerManager::calculate_voltage_limit(uint32_t voltage_mv) {
    if (voltage_mv < config_.vbat_min_mv) {
        state_.limited_by_voltage = true;
        return 0.3; // Voltaje muy bajo
    } else if (voltage_mv < config_.vbat_min_mv + 5000) {
        state_.limited_by_voltage = true;
        return 0.6;
    } else if (voltage_mv > config_.vbat_max_mv) {
        state_.limited_by_voltage = true;
        return 0.5; // Voltaje muy alto
    }
    
    state_.limited_by_voltage = false;
    return 1.0;
}

double PowerManager::apply_power_ramp(double demanded) {
    auto now = std::chrono::steady_clock::now();
    double elapsed_s = std::chrono::duration<double>(now - last_ramp_time_).count();
    
    if (elapsed_s <= 0) return demanded;
    
    double max_change = config_.power_ramp_rate * elapsed_s;
    double delta = demanded - previous_demanded_power_;
    
    // Limitar cambio de potencia
    if (std::abs(delta) > max_change) {
        demanded = previous_demanded_power_ + (delta > 0 ? max_change : -max_change);
    }
    
    previous_demanded_power_ = demanded;
    last_ramp_time_ = now;
    
    return demanded;
}

void PowerManager::distribute_power() {
    // Calcular potencia por motor
    double total_power = state_.available_power_w;
    
    // Por defecto, distribución equitativa
    double per_motor = total_power / 4.0;
    
    // Aplicar límites por motor
    for (size_t i = 0; i < 4; i++) {
        double max_per_motor = config_.max_power_w / 4.0; // Por ahora igual
        state_.motor_power_w[i] = std::min(per_motor, max_per_motor);
    }
    
    state_.motors_power_w = total_power;
}

PowerResult PowerManager::calculate_power(double accelerator,
                                        double brake,
                                        double soc,
                                        uint32_t battery_voltage_mv,
                                        int32_t battery_current_ma,
                                        const std::array<double, 4>& motor_temps) {
    // Reset estado
    state_.limited_by_soc = false;
    state_.limited_by_temperature = false;
    state_.limited_by_voltage = false;
    state_.limited_by_power = false;
    state_.degraded_mode = false;
    state_.limit_reason = "none";
    state_.current_soc = soc;
    state_.timestamp = std::chrono::steady_clock::now();
    
    // 1. Calcular potencia demandada
    if (brake > 0.1 && accelerator < 0.1) {
        // Frenada regenerativa
        state_.demanded_power_w = -brake * config_.max_battery_power_w * 0.5; // Solo 50% de recuperación
    } else {
        state_.demanded_power_w = accelerator * config_.max_power_w;
    }
    
    // Aplicar rampa de potencia
    state_.demanded_power_w = apply_power_ramp(state_.demanded_power_w);
    
    // 2. Calcular factores de limitación
    double soc_factor = calculate_soc_limit(soc);
    double temp_factor = calculate_temperature_limit(motor_temps);
    double voltage_factor = calculate_voltage_limit(battery_voltage_mv);
    
    // 3. Determinar factor de limitación más restrictivo
    power_limit_factor_ = std::min({soc_factor, temp_factor, voltage_factor});
    
    // 4. Calcular potencia disponible
    state_.available_power_w = state_.demanded_power_w * power_limit_factor_;
    
    // 5. Determinar resultado y causa
    if (power_limit_factor_ < 0.3) {
        state_.degraded_mode = true;
        state_.limit_reason = "critical";
        last_result_ = PowerResult::DEGRADED_MODE;
    } else if (state_.limited_by_soc) {
        state_.limit_reason = "soc";
        last_result_ = PowerResult::LIMITED_BY_SOC;
    } else if (state_.limited_by_temperature) {
        state_.limit_reason = "temperature";
        last_result_ = PowerResult::LIMITED_BY_TEMP;
    } else if (state_.limited_by_voltage) {
        state_.limit_reason = "voltage";
        last_result_ = PowerResult::LIMITED_BY_VOLTAGE;
    } else if (power_limit_factor_ < 1.0) {
        state_.limited_by_power = true;
        state_.limit_reason = "power";
        last_result_ = PowerResult::LIMITED_BY_POWER;
    } else {
        last_result_ = PowerResult::OK;
    }
    
    // 6. Calcular potencia de batería
    if (state_.available_power_w > 0) {
        // Potencia positiva: de batería a motores
        state_.battery_power_w = state_.available_power_w;
        state_.motors_power_w = state_.available_power_w;
        state_.auxiliary_power_w = config_.auxiliary_power_w;
    } else {
        // Frenada regenerativa
        state_.battery_power_w = state_.available_power_w * 0.5; // 50% eficiencia
        state_.motors_power_w = state_.available_power_w;
        state_.auxiliary_power_w = 0;
    }
    
    // 7. Distribuir potencia entre motores
    distribute_power();
    
    // Log si hay limitación
    if (last_result_ != PowerResult::OK) {
        LOG_WARN("PowerManager", "Power limited: " + state_.limit_reason + 
                 " factor=" + std::to_string(power_limit_factor_) +
                 " available=" + std::to_string(state_.available_power_w) + "W");
    }
    
    return last_result_;
}

double PowerManager::get_motor_power_limit(size_t motor_index) const {
    if (motor_index >= 4) return 0.0;
    return state_.motor_power_w[motor_index];
}

std::string PowerManager::dump_status() const {
    std::ostringstream out;
    out << "=== Power Manager Status ===" << std::endl;
    out << "Demanded: " << state_.demanded_power_w << " W" << std::endl;
    out << "Available: " << state_.available_power_w << " W" << std::endl;
    out << "Limit factor: " << power_limit_factor_ << std::endl;
    out << "Battery: " << state_.battery_power_w << " W" << std::endl;
    out << "Motors: " << state_.motors_power_w << " W" << std::endl;
    out << "Aux: " << state_.auxiliary_power_w << " W" << std::endl;
    out << "Mode: " << result_to_string(last_result_) << std::endl;
    out << "Reason: " << state_.limit_reason << std::endl;
    out << "SOC: " << state_.current_soc << "%" << std::endl;
    out << "M1: " << state_.motor_power_w[0] << " W" << std::endl;
    out << "M2: " << state_.motor_power_w[1] << " W" << std::endl;
    out << "M3: " << state_.motor_power_w[2] << " W" << std::endl;
    out << "M4: " << state_.motor_power_w[3] << " W" << std::endl;
    
    return out.str();
}

std::string PowerManager::result_to_string(PowerResult result) const {
    switch (result) {
        case PowerResult::OK: return "OK";
        case PowerResult::LIMITED_BY_SOC: return "LIMITED_BY_SOC";
        case PowerResult::LIMITED_BY_TEMP: return "LIMITED_BY_TEMP";
        case PowerResult::LIMITED_BY_VOLTAGE: return "LIMITED_BY_VOLTAGE";
        case PowerResult::LIMITED_BY_POWER: return "LIMITED_BY_POWER";
        case PowerResult::DEGRADED_MODE: return "DEGRADED_MODE";
        case PowerResult::EMERGENCY_STOP: return "EMERGENCY_STOP";
        default: return "UNKNOWN";
    }
}

} // namespace common
