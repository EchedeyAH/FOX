#pragma once
/**
 * error_system.hpp
 * Sistema de errores robusto y estructurado para ECU ATC8110
 * 
 * Reemplaza el sistema legacy de 3 niveles (leves/graves/críticos)
 * con contadores, debounce y watchdogs.
 * 
 * Equivalencias con legacy (err_fox.c, constantes_fox.h):
 *   ERL_* → ErrorLevel::LEVE
 *   ERG_* → ErrorLevel::GRAVE  
 *   ERC_* → ErrorLevel::CRITICO
 *   Watchdogs M1-M4 → Heartbeat monitoring
 *   Watchdog BMS → BMS communication monitoring
 */

#include <atomic>
#include <cstdint>
#include <string>
#include <array>
#include <chrono>

namespace common {

// ============================================================================
// NIVELES DE ERROR
// ============================================================================

enum class ErrorLevel : uint8_t {
    OK = 0,
    LEVE = 1,      // Warning - no acción inmediata
    GRAVE = 2,     // Error - requiere atención
    CRITICO = 3    // Critical - parada de emergencia
};

// ============================================================================
// SUBSISTEMAS DEL SISTEMA
// ============================================================================

enum class Subsystem : uint8_t {
    ECU = 0,
    BMS = 1,
    MOTOR1 = 2,
    MOTOR2 = 3,
    MOTOR3 = 4,
    MOTOR4 = 5,
    SUPERVISOR = 6,
    CAN_BUS = 7,
    SENSORES = 8,
    IMU = 9,
    POTENCIA = 10
};

// ============================================================================
// CÓDIGOS DE ERROR - Estructura: 16 bits
//   Bits 15-12: Subsistema (0-15)
//   Bits 11-8:  Categoría (0-15)  
//   Bits 7-0:  Código específico (0-255)
// ============================================================================

// Categorías de error
enum class ErrorCategory : uint8_t {
    COMUNICACION = 0x1,
    HARDWARE = 0x2,
    SOFTWARE = 0x3,
    SENSOR = 0x4,
    ACTUADOR = 0x5,
    POTENCIA = 0x6,
    TEMPERATURA = 0x7,
    TIMEOUT = 0x8,
    VALIDACION = 0x9,
    WATCHDOG = 0xA
};

// Macros para construir códigos de error
#define ERROR_CODE(subsys, cat, code) \
    ((((subsys) & 0x0F) << 12) | (((cat) & 0x0F) << 8) | ((code) & 0xFF))

// ============================================================================
// ERRORES DEFINIDOS - Equivalencias con legacy
// ============================================================================

namespace Errors {
    // --- Errores ECU ---
    constexpr uint16_t ECU_HILOS = ERROR_CODE(0, 0x3, 0x01);  // ERC_ECU_HILOS
    constexpr uint16_t ECU_COM_TAD = ERROR_CODE(0, 0x1, 0x02); // ERC_ECU_COM_TAD
    constexpr uint16_t ECU_COM_TAD_AO = ERROR_CODE(0, 0x1, 0x03);
    constexpr uint16_t ECU_COM_TCAN1 = ERROR_CODE(0, 0x1, 0x04);
    constexpr uint16_t ECU_COM_TCAN2 = ERROR_CODE(0, 0x1, 0x05);
    
    // --- Errores BMS ---
    constexpr uint16_t BMS_COM = ERROR_CODE(1, 0x1, 0x01);    // ERG_ECU_COM_BMS
    constexpr uint16_t BMS_TEMP = ERROR_CODE(1, 0x7, 0x02);    // ERG_BMS_TEMP_H
    constexpr uint16_t BMS_VOLT_HIGH = ERROR_CODE(1, 0x6, 0x03);
    constexpr uint16_t BMS_VOLT_LOW = ERROR_CODE(1, 0x6, 0x04);
    constexpr uint16_t BMS_CORRIENTE = ERROR_CODE(1, 0x6, 0x05);
    constexpr uint16_t BMS_CHASIS = ERROR_CODE(1, 0x2, 0x06);   // ERC_BMS_CHAS_CON
    constexpr uint16_t BMS_CEL_COM = ERROR_CODE(1, 0x1, 0x07); // ERC_BMS_CEL_COM
    constexpr uint16_t BMS_SYS_ERR = ERROR_CODE(1, 0x2, 0x08); // ERC_BMS_SYS_ERR
    
    // --- Errores Motores (M1-M4) ---
    constexpr uint16_t MOTOR_COM_BASE = 0x0200;
    constexpr uint16_t MOTOR_VOLT_HIGH = ERROR_CODE(2, 0x6, 0x01);  // ERG_M1_VOLT_H
    constexpr uint16_t MOTOR_VOLT_LOW = ERROR_CODE(2, 0x6, 0x02);   // ERG_M1_VOLT_L
    constexpr uint16_t MOTOR_TEMP_HIGH = ERROR_CODE(2, 0x7, 0x03);  // ERG_M1_TEMP_H
    constexpr uint16_t MOTOR_DIFF_PWR = ERROR_CODE(2, 0x6, 0x04);   // ERG_M1_DIFF_PWR
    constexpr uint16_t MOTOR_VAUX = ERROR_CODE(2, 0x6, 0x05);       // ERL_M1_VAUX
    
    // --- Errores Supervisor ---
    constexpr uint16_t SUPERV_HB_TIMEOUT = ERROR_CODE(6, 0x8, 0x01);
    constexpr uint16_t SUPERV_OFF = ERROR_CODE(6, 0x3, 0x02);
    
    // --- Errores CAN ---
    constexpr uint16_t CAN_TX_FAIL = ERROR_CODE(7, 0x1, 0x01);
    constexpr uint16_t CAN_RX_TIMEOUT = ERROR_CODE(7, 0x8, 0x02);
    constexpr uint16_t CAN_FRAME_INVALID = ERROR_CODE(7, 0x9, 0x03);
    
    // --- Errores Sensores ---
    constexpr uint16_t SENSOR_SUSP_DD = ERROR_CODE(8, 0x4, 0x01); // ERL_SENS_SUSP_DD
    constexpr uint16_t SENSOR_SUSP_DI = ERROR_CODE(8, 0x4, 0x02);
    constexpr uint16_t SENSOR_SUSP_TD = ERROR_CODE(8, 0x4, 0x03);
    constexpr uint16_t SENSOR_SUSP_TI = ERROR_CODE(8, 0x4, 0x04);
    constexpr uint16_t SENSOR_VOLANT = ERROR_CODE(8, 0x4, 0x05);
    constexpr uint16_t SENSOR_FRENO = ERROR_CODE(8, 0x4, 0x06);
    constexpr uint16_t SENSOR_ACEL = ERROR_CODE(8, 0x4, 0x07);
    constexpr uint16_t SENSOR_IMU = ERROR_CODE(9, 0x4, 0x01);     // ERL_SENS_IMU
}

// ============================================================================
// ESTRUCTURA DE ERROR INDIVIDUAL
// ============================================================================

struct ErrorEvent {
    uint16_t code{0};                    // Código de error
    ErrorLevel level{ErrorLevel::OK};     // Nivel
    Subsystem subsystem{Subsystem::ECU};   // Subsistema
    uint32_t count{0};                    // Contador de occurrences
    uint32_t reset_count{0};              // Contador para auto-reset
    bool active{false};                   // Error activo
    bool latched{false};                 // Error latcheado (requiere reset manual)
    std::chrono::steady_clock::time_point first_occurrence;
    std::chrono::steady_clock::time_point last_occurrence;
    std::string description;              // Descripción legible
};

// ============================================================================
// CONFIGURACIÓN DE ERRORES
// ============================================================================

struct ErrorConfig {
    // Thresholds de contadores (valores por defecto - legacy)
    uint8_t max_leve_count{10};      // Errores leves antes de escalar a grave
    uint8_t max_grave_count{5};     // Errores graves antes de escalar a crítico
    uint8_t debounce_count{3};      // Confirmaciones necesarias para activar
    
    // Timeouts (ms)
    uint32_t hb_timeout_ms{500};     // Timeout heartbeat supervisor
    uint32_t motor_hb_timeout_ms{2000}; // Timeout heartbeat motor (legacy: TIEMPO_EXP_MOT_S)
    uint32_t bms_timeout_ms{1000};   // Timeout mensajes BMS
    
    // Temperaturas (°C) - legacy constants
    int8_t motor_temp_leve{60};      // MOTOR_TEMP_ALTA_LEVE
    int8_t motor_temp_grave{80};     // MOTOR_TEMP_ALTA_GRAVE
    int8_t bms_temp_leve{50};
    int8_t bms_temp_grave{65};
    
    // Voltajes (V) - legacy constants
    uint8_t motor_v_aux_low_leve{4};   // MOTOR_V_AUX_L_LEVE
    uint8_t motor_v_aux_high_leve{6};  // MOTOR_V_AUX_H_LEVE
    uint8_t motor_v_bat_low_leve{60};  // MOTOR_V_BAJA_LEVE
    uint8_t motor_v_bat_low_grave{50}; // MOTOR_V_BAJA_GRAVE
    uint8_t motor_v_bat_high_leve{90}; // MOTOR_V_ALTA_LEVE
    uint8_t motor_v_bat_high_grave{95};// MOTOR_V_ALTA_GRAVE
};

// ============================================================================
// MANAGER DE ERRORES
// ============================================================================

class ErrorManager {
public:
    ErrorManager();
    
    // === API Principal ===
    
    /**
     * Registra un error en el sistema
     * @param code Código de error
     * @param level Nivel forzado (opcional, deduce del código si no se indica)
     * @return true si el error está activo tras el registro
     */
    bool raise_error(uint16_t code, ErrorLevel level = ErrorLevel::OK);
    
    /**
     * Limpia un error específico
     */
    void clear_error(uint16_t code);
    
    /**
     * Limpia todos los errores
     */
    void clear_all();
    
    /**
     * Obtiene el nivel de error más grave activo
     */
    ErrorLevel get_system_level() const;
    
    /**
     * Obtiene el estado completo del sistema de errores
     */
    void get_system_status(ErrorLevel& level, uint16_t& active_errors, 
                          uint32_t& total_errors) const;
    
    // === Watchdog Management ===
    
    /**
     * Actualiza timestamp de heartbeat de un componente
     */
    void update_heartbeat(Subsystem subsystem);
    
    /**
     * Verifica todos los watchdogs y genera errores si hay timeout
     * Debe llamarse periódicamente (ej: cada 100ms en hilo watchdog)
     * @return true si hay errores críticos detectados
     */
    bool check_watchdogs();
    
    /**
     * Obtiene tiempo desde último heartbeat de un componente
     */
    uint32_t get_time_since_heartbeat(Subsystem subsystem) const;
    
    // === Métricas y Debug ===
    
    /**
     * Dump de todos los errores activos
     */
    std::string dump_active_errors() const;
    
    /**
     * Obtiene contador de un error específico
     */
    uint32_t get_error_count(uint16_t code) const;
    
    /**
     * Configuración
     */
    void set_config(const ErrorConfig& config) { config_ = config; }
    const ErrorConfig& get_config() const { return config_; }

private:
    ErrorConfig config_;
    std::array<ErrorEvent, 32> errors_;  // Pool de errores
    std::atomic<uint32_t> total_errors_{0};
    
    // Heartbeat timestamps
    std::array<std::chrono::steady_clock::time_point, 11> last_heartbeat_;
    std::atomic<bool> heartbeat_initialized_{false};
    
    // Helpers
    ErrorEvent* find_error(uint16_t code);
    const ErrorEvent* find_error(uint16_t code) const;
    ErrorLevel deduce_level(uint16_t code) const;
    void log_error_event(const ErrorEvent& err, ErrorLevel new_level);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline const char* error_level_str(ErrorLevel level) {
    switch (level) {
        case ErrorLevel::OK: return "OK";
        case ErrorLevel::LEVE: return "WARNING";
        case ErrorLevel::GRAVE: return "ERROR";
        case ErrorLevel::CRITICO: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

inline const char* subsystem_str(Subsystem subsys) {
    switch (subsys) {
        case Subsystem::ECU: return "ECU";
        case Subsystem::BMS: return "BMS";
        case Subsystem::MOTOR1: return "M1";
        case Subsystem::MOTOR2: return "M2";
        case Subsystem::MOTOR3: return "M3";
        case Subsystem::MOTOR4: return "M4";
        case Subsystem::SUPERVISOR: return "SUPERV";
        case Subsystem::CAN_BUS: return "CAN";
        case Subsystem::SENSORES: return "SENSORS";
        case Subsystem::IMU: return "IMU";
        case Subsystem::POTENCIA: return "POWER";
        default: return "UNKNOWN";
    }
}

} // namespace common
