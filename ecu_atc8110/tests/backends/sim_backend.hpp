#pragma once
/**
 * sim_backend.hpp
 * Backend de simulación usando CanSimulator
 * 
 * Implementa ICanBackend para pruebas en modo SIM:
 * - Generación determinista de mensajes
 * - Inyección de errores controlados
 * - Control de tiempo simulado
 * 
 * Uso:
 *   BackendConfig cfg;
 *   cfg.backend_type = "sim";
 *   auto backend = create_can_backend(cfg);
 *   backend->start();
 */

#include "ican_backend.hpp"
#include "../../common/can_simulator.hpp"
#include "../../common/logging.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>

namespace ecu {
namespace testing {

// ============================================================================
// SIM BACKEND
// ============================================================================

/**
 * Backend de simulación para pruebas
 * Usa CanSimulator internamente
 */
class SimBackend : public ICanBackend {
public:
    explicit SimBackend(const BackendConfig& config);
    ~SimBackend() override;

    // ─────────────────────────────────────────────────────────────────────
    // ICanBackend Implementation
    // ─────────────────────────────────────────────────────────────────────

    bool initialize() override;
    bool start() override;
    void stop() override;
    bool is_running() const override;

    bool send(const CanFrame& frame) override;
    std::optional<CanFrame> receive() override;
    std::vector<CanFrame> receive_all() override;
    bool set_filter(uint32_t can_id, uint32_t can_mask) override;
    void clear_filters() override;

    BackendStats get_stats() const override;
    std::string get_type() const override { return "sim"; }
    std::string get_interface_name() const override { return "simulator"; }
    uint64_t get_time_us() const override;

    // ─────────────────────────────────────────────────────────────────────
    // Métodos específicos del backend SIM
    // ─────────────────────────────────────────────────────────────────────

    /// Configurar modo de simulación
    void set_simulation_mode(common::SimulationMode mode);

    /// Configurar parámetros de simulación
    void set_config(const common::SimulationConfig& config);

    /// Obtener configuración actual
    common::SimulationConfig get_config() const;

    /// Obtener modo de simulación
    common::SimulationMode get_simulation_mode() const;

    /// Inyectar mensaje de motor manualmente
    void inject_motor_message(uint8_t motor_idx, const std::vector<uint8_t>& data);

    /// Inyectar mensaje BMS manualmente
    void inject_bms_message(const std::vector<uint8_t>& data);

    /// Provocar timeout de motor
    void trigger_motor_timeout(uint8_t motor_idx);

    /// Recuperar motor de timeout
    void recover_motor(uint8_t motor_idx);

    /// Registrar callback para mensajes salientes (para verificar TX)
    using TxCallback = std::function<void(const CanFrame&)>;
    void set_tx_callback(TxCallback cb);

    /// Obtener contador de mensajes de motor
    uint32_t get_motor_msg_count() const;

    /// Obtener contador de mensajes BMS
    uint32_t get_bms_msg_count() const;

private:
    // Configuración
    BackendConfig config_;

    // Simulator reference
    common::CanSimulator* simulator_ = nullptr;
    bool owns_simulator_ = false;

    // Estado
    std::atomic<bool> running_;
    std::atomic<uint64_t> simulated_time_us_;

    // Filtros
    uint32_t filter_can_id_ = 0;
    uint32_t filter_can_mask_ = 0x7FF;
    bool filter_enabled_ = true;

    // Buffers para mensajes
    std::queue<CanFrame> rx_queue_;
    mutable std::mutex rx_mutex_;

    // Callbacks
    TxCallback tx_callback_;
    CanReceiveCallback can_callback_;
    CanErrorCallback error_callback_;

    // Estadísticas
    BackendStats stats_;

    // Thread para recepción
    std::thread receive_thread_;
    std::atomic<bool> thread_running_;

    // Helpers
    void setup_simulator_callbacks();
    void receive_loop();
    bool passes_filter(uint32_t can_id) const;
    CanFrame convert_to_frame(const common::SimCanMessage& sim_msg);
    common::SimulationMode current_sim_mode_;
};

// ============================================================================
// IMPLEMENTACIÓN
// ============================================================================

inline SimBackend::SimBackend(const BackendConfig& config)
    : config_(config)
    , running_(false)
    , simulated_time_us_(0)
    , current_sim_mode_(common::SimulationMode::DISABLED)
    , thread_running_(false)
{
    // Crear instancia del simulator
    simulator_ = &common::get_can_simulator();
    owns_simulator_ = false;
}

inline SimBackend::~SimBackend() {
    stop();
    if (owns_simulator_ && simulator_) {
        delete simulator_;
    }
}

inline bool SimBackend::initialize() {
    if (running_.load()) {
        return true;
    }

    // Configurar filtros por defecto
    if (config_.accept_can_ids.empty()) {
        // Aceptar IDs estándar de motores y BMS
        config_.accept_can_ids = {0x281, 0x282, 0x283, 0x284, 0x04D};
    }

    LOG_INFO("SIM_BACKEND", "Backend SIM inicializado");
    return true;
}

inline bool SimBackend::start() {
    if (running_.load()) {
        return true;
    }

    // Iniciar simulator
    simulator_->start();
    simulator_->set_mode(common::SimulationMode::NORMAL);

    // Configurar callbacks
    setup_simulator_callbacks();

    running_ = true;
    simulated_time_us_ = 0;

    // Iniciar thread de recepción
    thread_running_ = true;
    receive_thread_ = std::thread(&SimBackend::receive_loop, this);

    LOG_INFO("SIM_BACKEND", "Backend SIM iniciado");
    return true;
}

inline void SimBackend::stop() {
    if (!running_.load()) {
        return;
    }

    running_ = false;
    thread_running_ = false;

    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }

    simulator_->stop();

    LOG_INFO("SIM_BACKEND", "Backend SIM detenido");
}

inline bool SimBackend::is_running() const {
    return running_.load();
}

inline bool SimBackend::send(const CanFrame& frame) {
    if (!running_.load()) {
        LOG_ERROR("SIM_BACKEND", "Backend no está corriendo");
        return false;
    }

    // En SIM, el "send" simula que el mensaje fue procesado
    // Por ahora solo registramos en estadísticas
    stats_.tx_count++;
    stats_.last_tx_timestamp_us = simulated_time_us_.load();

    // Notificar callback TX
    if (tx_callback_) {
        tx_callback_(frame);
    }

    LOG_DEBUG("SIM_BACKEND", "TX simulado: ID=0x" + 
              std::to_string(frame.id) + 
              " dlc=" + std::to_string(frame.payload.size()));

    return true;
}

inline std::optional<CanFrame> SimBackend::receive() {
    std::lock_guard<std::mutex> lock(rx_mutex_);
    
    if (rx_queue_.empty()) {
        return std::nullopt;
    }

    auto frame = rx_queue_.front();
    rx_queue_.pop();
    return frame;
}

inline std::vector<CanFrame> SimBackend::receive_all() {
    std::lock_guard<std::mutex> lock(rx_mutex_);
    
    std::vector<CanFrame> frames;
    while (!rx_queue_.empty()) {
        frames.push_back(rx_queue_.front());
        rx_queue_.pop();
    }
    return frames;
}

inline bool SimBackend::set_filter(uint32_t can_id, uint32_t can_mask) {
    filter_can_id_ = can_id;
    filter_can_mask_ = can_mask;
    filter_enabled_ = true;
    
    LOG_INFO("SIM_BACKEND", "Filtro configurado: ID=0x" + 
             std::to_string(can_id) + " Mask=0x" + std::to_string(can_mask));
    return true;
}

inline void SimBackend::clear_filters() {
    filter_enabled_ = false;
    LOG_INFO("SIM_BACKEND", "Filtros limpiados");
}

inline BackendStats SimBackend::get_stats() const {
    // Actualizar con stats del simulator
    stats_.rx_count = simulator_->get_motor_msg_count() + simulator_->get_bms_msg_count();
    return stats_;
}

inline uint64_t SimBackend::get_time_us() const {
    return simulated_time_us_.load();
}

// ─────────────────────────────────────────────────────────────────────
// Métodos específicos del backend SIM
// ─────────────────────────────────────────────────────────────────────

inline void SimBackend::set_simulation_mode(common::SimulationMode mode) {
    current_sim_mode_ = mode;
    if (running_.load()) {
        simulator_->set_mode(mode);
    }
}

inline void SimBackend::set_config(const common::SimulationConfig& config) {
    simulator_->set_config(config);
}

inline common::SimulationConfig SimBackend::get_config() const {
    return simulator_->get_config();
}

inline common::SimulationMode SimBackend::get_simulation_mode() const {
    return current_sim_mode_;
}

inline void SimBackend::inject_motor_message(uint8_t motor_idx, 
                                              const std::vector<uint8_t>& data) {
    simulator_->inject_motor_message(motor_idx, data);
}

inline void SimBackend::inject_bms_message(const std::vector<uint8_t>& data) {
    simulator_->inject_bms_message(data);
}

inline void SimBackend::trigger_motor_timeout(uint8_t motor_idx) {
    simulator_->trigger_motor_timeout(motor_idx);
}

inline void SimBackend::recover_motor(uint8_t motor_idx) {
    simulator_->recover_motor(motor_idx);
}

inline void SimBackend::set_tx_callback(TxCallback cb) {
    tx_callback_ = std::move(cb);
}

inline uint32_t SimBackend::get_motor_msg_count() const {
    return simulator_->get_motor_msg_count();
}

inline uint32_t SimBackend::get_bms_msg_count() const {
    return simulator_->get_bms_msg_count();
}

// ─────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────

inline void SimBackend::setup_simulator_callbacks() {
    // Callback para mensajes de motores
    simulator_->set_motor_callback([this](const common::SimCanMessage& sim_msg) {
        auto frame = convert_to_frame(sim_msg);
        
        // Aplicar filtro
        if (passes_filter(frame.id)) {
            std::lock_guard<std::mutex> lock(rx_mutex_);
            rx_queue_.push(frame);
            stats_.rx_count++;
            stats_.last_rx_timestamp_us = simulated_time_us_.load();
            
            // Notificar callback
            if (can_callback_) {
                can_callback_(frame);
            }
        }
    });

    // Callback para mensajes BMS
    simulator_->set_bms_callback([this](const common::SimCanMessage& sim_msg) {
        auto frame = convert_to_frame(sim_msg);
        
        if (passes_filter(frame.id)) {
            std::lock_guard<std::mutex> lock(rx_mutex_);
            rx_queue_.push(frame);
            stats_.rx_count++;
            stats_.last_rx_timestamp_us = simulated_time_us_.load();
            
            if (can_callback_) {
                can_callback_(frame);
            }
        }
    });
}

inline void SimBackend::receive_loop() {
    while (thread_running_.load()) {
        // En SIM, los mensajes llegan via callbacks
        // Este loop es para mantener la estructura
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        // Actualizar tiempo simulado
        simulated_time_us_ += 1000;  // 1ms por iteración
    }
}

inline bool SimBackend::passes_filter(uint32_t can_id) const {
    if (!filter_enabled_) {
        return true;
    }
    
    // Verificar contra filtro
    if ((can_id & filter_can_mask_) == (filter_can_id_ & filter_can_mask_)) {
        return true;
    }
    
    // Verificar contra lista de IDs aceptados
    for (uint32_t accept_id : config_.accept_can_ids) {
        if (can_id == accept_id) {
            return true;
        }
    }
    
    return false;
}

inline CanFrame SimBackend::convert_to_frame(const common::SimCanMessage& sim_msg) {
    CanFrame frame;
    frame.id = sim_msg.id;
    frame.payload = sim_msg.data;
    frame.timestamp_us = sim_msg.timestamp_ms * 1000;
    frame.valid = sim_msg.valid;
    return frame;
}

} // namespace testing
} // namespace ecu
