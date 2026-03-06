#pragma once
/**
 * ican_backend.hpp
 * Interfaz abstracta para backends de comunicación CAN
 * 
 * Permite abstracción entre:
 * - Backend SIM: CanSimulator para pruebas
 * - Backend REAL: SocketCAN para ECU real
 * 
 * Uso:
 *   std::unique_ptr<ICanBackend> backend = create_backend("sim");
 *   backend->send(frame);
 *   auto rx = backend->receive();
 */

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ecu {
namespace testing {

// ============================================================================
// TIPOS
// ============================================================================

/**
 * Frame CAN genérico (independiente del backend)
 */
struct CanFrame {
    uint32_t id = 0;
    std::vector<uint8_t> payload;
    uint64_t timestamp_us = 0;
    bool extended = false;
    bool valid = true;
};

/**
 * Estadísticas del backend
 */
struct BackendStats {
    uint64_t tx_count = 0;
    uint64_t rx_count = 0;
    uint64_t tx_errors = 0;
    uint64_t rx_errors = 0;
    uint64_t bus_off_count = 0;
    uint64_t last_tx_timestamp_us = 0;
    uint64_t last_rx_timestamp_us = 0;
};

/**
 * Configuración del backend
 */
struct BackendConfig {
    std::string backend_type = "sim";      // "sim" o "socketcan"
    std::string if0 = "emuccan0";           // Interfaz CAN 0
    std::string if1 = "emuccan1";           // Interfaz CAN 1
    uint32_t loopback_ms = 10;            // Intervalo de polling para REAL
    bool filter_can_ids = true;            // Aplicar filtros
    std::vector<uint32_t> accept_can_ids;  // Lista de IDs a aceptar
};

// ============================================================================
// INTERFAZ I CAN BACKEND
// ============================================================================

/**
 * Interfaz abstracta para backends CAN
 * Implementada por SimBackend y SocketCanBackend
 */
class ICanBackend {
public:
    virtual ~ICanBackend() = default;

    // ─────────────────────────────────────────────────────────────────────
    // CONTROL
    // ─────────────────────────────────────────────────────────────────────

    /// Inicializar el backend
    virtual bool initialize() = 0;

    /// Iniciar comunicación
    virtual bool start() = 0;

    /// Detener comunicación
    virtual void stop() = 0;

    /// Verificar si está activo
    virtual bool is_running() const = 0;

    // ─────────────────────────────────────────────────────────────────────
    // OPERACIONES CAN
    // ─────────────────────────────────────────────────────────────────────

    /// Enviar frame CAN
    virtual bool send(const CanFrame& frame) = 0;

    /// Recibir frame CAN (non-blocking)
    virtual std::optional<CanFrame> receive() = 0;

    /// Recibir todos los frames disponibles
    virtual std::vector<CanFrame> receive_all() = 0;

    /// Configurar filtro de IDs CAN
    virtual bool set_filter(uint32_t can_id, uint32_t can_mask) = 0;

    /// Limpiar filtros
    virtual void clear_filters() = 0;

    // ─────────────────────────────────────────────────────────────────────
    // CONSULTAS
    // ─────────────────────────────────────────────────────────────────────

    /// Obtener estadísticas
    virtual BackendStats get_stats() const = 0;

    /// Obtener tipo de backend
    virtual std::string get_type() const = 0;

    /// Obtener nombre de interfaz
    virtual std::string get_interface_name() const = 0;

    /// Obtener tiempo actual del backend (simulado o real)
    virtual uint64_t get_time_us() const = 0;
};

// ============================================================================
// CALLBACKS
// ============================================================================

/**
 * Callback para recepción de frames
 */
using CanReceiveCallback = std::function<void(const CanFrame&)>;

/**
 * Callback para errores
 */
using CanErrorCallback = std::function<void(const std::string& error)>;

// ============================================================================
// FACTORY
// ============================================================================

/**
 * Crea una instancia del backend especificado
 */
std::unique_ptr<ICanBackend> create_can_backend(const BackendConfig& config);

} // namespace testing
} // namespace ecu
