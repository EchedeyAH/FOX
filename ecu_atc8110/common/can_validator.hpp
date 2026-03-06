#pragma once
/**
 * can_validator.hpp
 * Validación de comunicaciones CAN para ECU ATC8110
 * 
 * Funcionalidades:
 * - Timeout por mensaje esperado
 * - Reintentos automáticos
 * - Detección de pérdida de bus
 * - Detección de frames inválidos
 * - Conteo de mensajes por segundo (bus load)
 * 
 * Equivalencias legacy:
 *   TIEMPO_EXP_MOT_S → motor_response_timeout_ms
 *   TIEMPO_EXP_CAN_X → max_tx_retries
 */

#include <atomic>
#include <cstdint>
#include <array>
#include <chrono>
#include <functional>

namespace common {

// ============================================================================
// CONFIGURACIÓN DE VALIDACIÓN CAN
// ============================================================================

struct CanValidatorConfig {
    // Timeouts (ms)
    uint32_t motor_response_timeout_ms{2000};  // Legacy: TIEMPO_EXP_MOT_S
    uint32_t bms_response_timeout_ms{1000};
    uint32_t supervisor_response_timeout_ms{500};
    uint32_t bus_timeout_ms{5000};              // Pérdida completa del bus
    
    // Reintentos
    uint8_t max_tx_retries{3};                 // Legacy: TIEMPO_EXP_CAN_X
    uint8_t max_rx_retries{2};
    
    // Validación de datos
    bool validate_ranges{true};
    bool validate_crc{false};                   // Si el protocolo tiene CRC
    
    // Límites de tasa de mensajes
    uint16_t min_msg_rate_hz{10};               // Mínimo msgs/segundo
    uint16_t max_msg_rate_hz{10000};           // Máximo (para detectar ruido)
    
    // Thresholds para errores
    uint8_t consecutive_timeouts_to_error{3};   // Timeouts consecutivos antes de error
    uint8_t max_consecutive_errors{5};          // Errores consecutivos antes de crítico
};

// ============================================================================
// RESULTADO DE VALIDACIÓN
// ============================================================================

enum class CanValidationResult : uint8_t {
    OK = 0,
    TIMEOUT = 1,
    RETRY_EXHAUSTED = 2,
    INVALID_FRAME = 3,
    INVALID_DATA = 4,
    BUS_OFF = 5,
    BUS_ERROR = 6,
    RATE_ERROR = 7
};

// ============================================================================
// ESTADO DE COMUNICACIÓN POR COMPONENTE
// ============================================================================

struct CanPeerStatus {
    // Estado de comunicación
    bool connected{false};
    bool responding{false};
    uint32_t messages_received{0};
    uint32_t messages_sent{0};
    uint32_t timeouts{0};
    uint32_t errors{0};
    uint32_t retries{0};
    
    // Timestamps
    std::chrono::steady_clock::time_point last_tx;
    std::chrono::steady_clock::time_point last_rx;
    std::chrono::steady_clock::time_point last_error;
    
    // Rate limiting
    uint16_t msgs_last_second{0};
    std::chrono::steady_clock::time_point rate_check_time;
    
    // Estado degraded
    bool degraded_mode{false};
    uint8_t consecutive_failures{0};
};

// ============================================================================
// CALLBACKS PARA EVENTOS
// ============================================================================

using CanTimeoutCallback = std::function<void(uint8_t motor_id)>;
using CanErrorCallback = std::function<void(CanValidationResult, const std::string&)>;
using CanBusOffCallback = std::function<void()>;

// ============================================================================
// VALIDADOR DE COMUNICACIONES CAN
// ============================================================================

class CanValidator {
public:
    CanValidator();
    
    // === Configuración ===
    void set_config(const CanValidatorConfig& config) { config_ = config; }
    const CanValidatorConfig& get_config() const { return config_; }
    
    // === Callbacks ===
    void set_timeout_callback(CanTimeoutCallback cb) { timeout_callback_ = cb; }
    void set_error_callback(CanErrorCallback cb) { error_callback_ = cb; }
    void set_bus_off_callback(CanBusOffCallback cb) { bus_off_callback_ = cb; }
    
    // === API de TX (envío de mensajes) ===
    
    /**
     * Registra el envío de un mensaje a un componente
     * @param motor_id ID del motor (1-4), 0 para BMS, 255 para supervisor
     * @return true si se debe reintentar
     */
    bool register_tx(uint8_t motor_id);
    
    /**
     * Registra éxito de transmisión
     */
    void register_tx_success(uint8_t motor_id);
    
    /**
     * Registra mensaje recibido de un componente
     * @param motor_id ID del motor (1-4), 0 para BMS, 255 para supervisor
     */
    void register_rx(uint8_t motor_id);
    
    /**
     * Valida un frame recibido
     * @return OK si válido, código de error si no
     */
    CanValidationResult validate_frame(const std::vector<uint8_t>& payload, 
                                       size_t expected_min_size) const;
    
    // === API de Estado ===
    
    /**
     * Obtiene el estado de comunicación de un componente
     */
    CanPeerStatus get_peer_status(uint8_t motor_id) const;
    
    /**
     * Obtiene el estado general del bus CAN
     */
    bool is_bus_healthy() const { return bus_healthy_.load(); }
    
    /**
     * Obtiene número total de errores de comunicación
     */
    uint32_t get_total_errors() const { return total_errors_.load(); }
    
    /**
     * Obtiene tasa de mensajes del último segundo
     */
    uint16_t get_message_rate() const { return messages_per_second_.load(); }
    
    // === Verificación periódica ===
    
    /**
     * Verifica timeouts y errores
     * Debe llamarse periódicamente (ej: cada 100ms desde hilo watchdog)
     * @return true si hay errores críticos
     */
    bool check_communication_health();
    
    /**
     * Resetea contadores de un componente
     */
    void reset_peer(uint8_t motor_id);
    
    /**
     * Resetea todo el validador
     */
    void reset_all();
    
    // === Métricas para debug/monitoring ===
    
    /**
     * Obtiene resumen del estado de comunicación
     */
    std::string dump_status() const;

private:
    CanValidatorConfig config_;
    
    // Estado por peer (0=BMS, 1-4=Motores, 5=Supervisor)
    std::array<CanPeerStatus, 6> peers_;
    
    // Estado global del bus
    std::atomic<bool> bus_healthy_{true};
    std::atomic<uint32_t> total_errors_{0};
    std::atomic<uint16_t> messages_per_second_{0};
    
    // Callbacks
    CanTimeoutCallback timeout_callback_;
    CanErrorCallback error_callback_;
    CanBusOffCallback bus_off_callback_;
    
    // Helpers
    CanPeerStatus& get_peer_ref(uint8_t motor_id);
    const CanPeerStatus* get_peer_ptr(uint8_t motor_id) const;
    void update_message_rate();
    void handle_timeout(CanPeerStatus& peer, uint8_t motor_id);
    void handle_error(CanPeerStatus& peer, CanValidationResult result);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline const char* can_validation_result_str(CanValidationResult result) {
    switch (result) {
        case CanValidationResult::OK: return "OK";
        case CanValidationResult::TIMEOUT: return "TIMEOUT";
        case CanValidationResult::RETRY_EXHAUSTED: return "RETRY_EXHAUSTED";
        case CanValidationResult::INVALID_FRAME: return "INVALID_FRAME";
        case CanValidationResult::INVALID_DATA: return "INVALID_DATA";
        case CanValidationResult::BUS_OFF: return "BUS_OFF";
        case CanValidationResult::BUS_ERROR: return "BUS_ERROR";
        case CanValidationResult::RATE_ERROR: return "RATE_ERROR";
        default: return "UNKNOWN";
    }
}

// Conveniencia: IDs de peer
namespace CanPeers {
    constexpr uint8_t BMS = 0;
    constexpr uint8_t MOTOR1 = 1;
    constexpr uint8_t MOTOR2 = 2;
    constexpr uint8_t MOTOR3 = 3;
    constexpr uint8_t MOTOR4 = 4;
    constexpr uint8_t SUPERVISOR = 5;
}

} // namespace common
