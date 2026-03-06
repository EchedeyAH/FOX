#pragma once
/**
 * power_manager.hpp
 * Gestión de potencia para ECU ATC8110
 * 
 * Funcionalidades:
 * - Cálculo de potencia demandada por el conductor
 * - Limitación por SOC (State of Charge)
 * - Limitación por temperatura
 * - Limitación por voltaje de batería
 * - Distribución de potencia entre motores
 * - Modo degradado cuando la potencia disponible es baja
 * 
 * Equivalencias con legacy (gestionpot_fox.c):
 *   pot_deman → demanded_power_w
 *   pot_bat_act/sig → battery_power_available
 *   pot_motores_act → motors_power_consumption
 *   POTMINFC, POTMAXFC → power_limits
 */

#include <array>
#include <cstdint>
#include <chrono>

namespace common {

// ============================================================================
// CONFIGURACIÓN DE GESTIÓN DE POTENCIA
// ============================================================================

struct PowerManagerConfig {
    // Potencia máxima del sistema (W)
    double max_power_w{56000};           // Legacy: POTMAXFC
    double min_power_w{1000};            // Legacy: POTMINFC
    
    // Potencia máxima de batería (W)
    double max_battery_power_w{18000};   // Legacy: POTMAXBAT
    double min_battery_power_w{-18000};  // Legacy: POTMAXBATR (recuperación)
    
    // Potencia auxiliar (W)
    double auxiliary_power_w{1000};      // Legacy: P_AUX
    
    // SOC limits (%)
    double soc_min_limit{5.0};           // legacy: BATT_M_B
    double soc_critical_limit{10.0};
    double soc_normal_limit{20.0};
    double soc_full_limit{95.0};
    
    // Temperatura límites (°C)
    double temp_bat_max{60.0};
    double temp_bat_limit{50.0};
    double temp_motor_max{80.0};
    double temp_motor_limit{65.0};
    
    // Voltaje pack (mV)
    uint32_t vbat_min_mv{50000};        // Legacy: MOTOR_V_BAJA_LEVE * 1000
    uint32_t vbat_max_mv{95000};        // Legacy: MOTOR_V_ALTA_GRAVE * 1000
    uint32_t vbat_nominal_mv{76000};    // 76S * 3.7V = ~281V nominal, ajustado para 24S legacy
    
    // Rampa de potencia (% por segundo)
    double power_ramp_rate{10.0};        // 10% por segundo para suavidad
    
    // Distribución de potencia por eje
    double front_axis_ratio{0.5};        // 50% frontal
    double rear_axis_ratio{0.5};         // 50% trasero
    
    // Factores de corrección
    double temperature_derating_factor{0.8};  // Cuando temp alta
    double soc_derating_factor{0.7};         // Cuando SOC bajo
};

// ============================================================================
// ESTADO DE POTENCIA
// ============================================================================

struct PowerState {
    // Potencia calculada
    double demanded_power_w{0.0};       // Potencia demandada por conductor
    double available_power_w{0.0};      // Potencia disponible (tras límites)
    double battery_power_w{0.0};        // Potencia de batería usada
    double motors_power_w{0.0};         // Potencia total motores
    double auxiliary_power_w{0.0};      // Potencia auxiliar
    
    // Distribución por motor
    std::array<double, 4> motor_power_w;  // Potencia por motor
    
    // Estado de limitación
    bool limited_by_soc{false};
    bool limited_by_temperature{false};
    bool limited_by_voltage{false};
    bool limited_by_power{false};
    bool degraded_mode{false};
    
    // Causa de limitación (para debug/logging)
    std::string limit_reason;
    
    // SOC actual
    double current_soc{50.0};
    
    // Timestamp
    std::chrono::steady_clock::time_point timestamp;
};

// ============================================================================
// RESULTADO DEL CÁLCULO DE POTENCIA
// ============================================================================

enum class PowerResult : uint8_t {
    OK = 0,
    LIMITED_BY_SOC = 1,
    LIMITED_BY_TEMP = 2,
    LIMITED_BY_VOLTAGE = 3,
    LIMITED_BY_POWER = 4,
    DEGRADED_MODE = 5,
    EMERGENCY_STOP = 6  // Parada de emergencia
};

// ============================================================================
// POWER MANAGER
// ============================================================================

class PowerManager {
public:
    PowerManager();
    
    // === Configuración ===
    void set_config(const PowerManagerConfig& config) { config_ = config; }
    const PowerManagerConfig& get_config() const { return config_; }
    
    // === API Principal ===
    
    /**
     * Calcula la potencia disponible y la distribución
     * @param accelerator Pedal acelerador (0.0 - 1.0)
     * @param brake Pedal freno (0.0 - 1.0)
     * @param soc Estado de carga (%) 
     * @param battery_voltage_mv Voltaje pack (mV)
     * @param battery_current_ma Corriente pack (mA)
     * @param motor_temps Temperatura de cada motor (°C)
     * @return Resultado del cálculo de potencia
     */
    PowerResult calculate_power(double accelerator,
                              double brake,
                              double soc,
                              uint32_t battery_voltage_mv,
                              int32_t battery_current_ma,
                              const std::array<double, 4>& motor_temps);
    
    /**
     * Obtiene el estado actual de potencia
     */
    PowerState get_state() const { return state_; }
    
    /**
     * Obtiene la potencia disponible para un motor específico
     * @param motor_index Índice del motor (0-3)
     */
    double get_motor_power_limit(size_t motor_index) const;
    
    /**
     * Obtiene el factor de limitación actual (0.0 - 1.0)
     * 1.0 = sin límite, 0.0 = potencia cero
     */
    double get_power_limit_factor() const { return power_limit_factor_; }
    
    /**
     * Verifica si el sistema está en modo degradado
     */
    bool is_degraded() const { return state_.degraded_mode; }
    
    /**
     * Obtiene el resultado del último cálculo
     */
    PowerResult get_last_result() const { return last_result_; }
    
    /**
     * Reset del gestor de potencia
     */
    void reset();
    
    // === Métricas para debug/monitoring ===
    
    /**
     * Obtiene resumen del estado de potencia
     */
    std::string dump_status() const;

private:
    PowerManagerConfig config_;
    PowerState state_;
    PowerResult last_result_{PowerResult::OK};
    
    double power_limit_factor_{1.0};    // Factor de limitación global
    double previous_demanded_power_{0.0};
    std::chrono::steady_clock::time_point last_ramp_time_;
    
    // Helpers
    double calculate_soc_limit(double soc);
    double calculate_temperature_limit(const std::array<double, 4>& motor_temps);
    double calculate_voltage_limit(uint32_t voltage_mv);
    double apply_power_ramp(double demanded);
    void distribute_power();
    std::string result_to_string(PowerResult result) const;
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline const char* power_result_str(PowerResult result) {
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
