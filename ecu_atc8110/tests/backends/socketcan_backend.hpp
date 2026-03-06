#pragma once
/**
 * socketcan_backend.hpp
 * Backend REAL para interfaces CAN físicas via SocketCAN
 * 
 * Implementa ICanBackend para pruebas en ECU real:
 * - Comunicación con interfaces reales (emuccan0, emuccan1)
 * - Soporte para filtros de ID CAN
 * - Estadísticas de errores
 * - Thread-safe
 * 
 * Uso:
 *   BackendConfig cfg;
 *   cfg.backend_type = "socketcan";
 *   cfg.if0 = "emuccan0";
 *   cfg.if1 = "emuccan1";
 *   auto backend = create_can_backend(cfg);
 *   backend->start();
 * 
 * Requisitos:
 *   - Permisos de root o pertenencia al grupo can
 *   - Interfaces CAN configuradas (ip link set up)
 */

#include "ican_backend.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <net/if.h>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// Linux SocketCAN headers
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <errno.h>

namespace ecu {
namespace testing {

// ============================================================================
// CONSTANTES
// ============================================================================

// IDs CAN típicos del sistema
constexpr uint32_t CAN_ID_MOTOR_1 = 0x281;
constexpr uint32_t CAN_ID_MOTOR_2 = 0x282;
constexpr uint32_t CAN_ID_MOTOR_3 = 0x283;
constexpr uint32_t CAN_ID_MOTOR_4 = 0x284;
constexpr uint32_t CAN_ID_BMS = 0x04D;

// Intervalo de polling por defecto
constexpr uint32_t DEFAULT_POLL_INTERVAL_MS = 10;

// ============================================================================
// SOCKETCAN BACKEND
// ============================================================================

/**
 * Backend para interfaces CAN reales via SocketCAN
 */
class SocketCanBackend : public ICanBackend {
public:
    explicit SocketCanBackend(const BackendConfig& config);
    ~SocketCanBackend() override;

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
    std::string get_type() const override { return "socketcan"; }
    std::string get_interface_name() const override { return config_.if0; }
    uint64_t get_time_us() const override;

    // ─────────────────────────────────────────────────────────────────────
    // Métodos específicos del backend SocketCAN
    // ─────────────────────────────────────────────────────────────────────

    /// Configurar segunda interfaz (para comunicación bidireccional)
    bool set_secondary_interface(const std::string& ifname);

    /// Obtener estadísticas de errores extendidas
    struct ExtendedErrorStats {
        uint32_t tx_errors;
        uint32_t rx_errors;
        uint32_t bus_off_count;
        uint32_t controller_errors;
        uint32_t protocol_errors;
    };
    ExtendedErrorStats get_extended_error_stats() const;

    /// Verificar si la interfaz está activa (link up)
    bool is_interface_up() const;

    /// Reiniciar interfaz después de BUS-OFF
    bool reset_interface();

    /// Establecer callback para errores
    using ErrorCallback = std::function<void(const std::string&, int)>;
    void set_error_callback(ErrorCallback cb);

    /// Obtener file descriptor del socket
    int get_socket_fd() const { return socket_fd_; }

private:
    // Configuración
    BackendConfig config_;

    // Socket
    int socket_fd_ = -1;
    int socket_fd_if1_ = -1;  // Segunda interfaz opcional

    // Estado
    std::atomic<bool> running_;
    mutable std::mutex stats_mutex_;

    // Buffers
    std::queue<CanFrame> rx_queue_;
    mutable std::mutex rx_mutex_;

    // Filtros
    std::vector<struct can_filter> filters_;
    bool filter_all_ = false;

    // Callbacks
    CanReceiveCallback can_callback_;
    ErrorCallback error_callback_;

    // Estadísticas
    BackendStats stats_;
    ExtendedErrorStats ext_error_stats_;

    // Thread
    std::thread receive_thread_;
    std::atomic<bool> thread_running_;

    // Helpers
    bool open_socket(const std::string& ifname, int& fd);
    void close_socket(int& fd);
    void receive_loop();
    bool process_can_frame(const struct can_frame& frame);
    CanFrame convert_to_frame(const struct can_frame& can_msg);
    uint64_t get_current_time_us() const;
};

// ============================================================================
// IMPLEMENTACIÓN
// ============================================================================

inline SocketCanBackend::SocketCanBackend(const BackendConfig& config)
    : config_(config)
    , running_(false)
    , thread_running_(false)
{
    // Por defecto, aceptar IDs de motores y BMS
    if (config_.accept_can_ids.empty()) {
        config_.accept_can_ids = {
            CAN_ID_MOTOR_1, CAN_ID_MOTOR_2, 
            CAN_ID_MOTOR_3, CAN_ID_MOTOR_4,
            CAN_ID_BMS
        };
    }
}

inline SocketCanBackend::~SocketCanBackend() {
    stop();
}

inline bool SocketCanBackend::initialize() {
    if (running_.load()) {
        return true;
    }

    // Abrir socket primario
    if (!open_socket(config_.if0, socket_fd_)) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Error abriendo interfaz " + config_.if0);
        return false;
    }

    // Abrir segunda interfaz si está configurada
    if (!config_.if1.empty() && config_.if1 != config_.if0) {
        if (!open_socket(config_.if1, socket_fd_if1_)) {
            LOG_WARN("SOCKETCAN_BACKEND", "No se pudo abrir interfaz secundaria " + config_.if1);
            // Continuar con solo una interfaz
            socket_fd_if1_ = -1;
        }
    }

    // Configurar filtros si está habilitado
    if (config_.filter_can_ids && !config_.accept_can_ids.empty()) {
        filters_.clear();
        for (uint32_t id : config_.accept_can_ids) {
            struct can_filter filter;
            filter.can_id = id;
            filter.can_mask = 0x7FF;  // Exact match para estándar
            filters_.push_back(filter);
        }

        if (!filters_.empty()) {
            if (setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, 
                          filters_.data(), filters_.size() * sizeof(struct can_filter)) < 0) {
                LOG_WARN("SOCKETCAN_BACKEND", "Error configurando filtros: " + 
                        std::string(strerror(errno)));
            }

            if (socket_fd_if1_ >= 0) {
                setsockopt(socket_fd_if1_, SOL_CAN_RAW, CAN_RAW_FILTER,
                          filters_.data(), filters_.size() * sizeof(struct can_filter));
            }
        }
    }

    LOG_INFO("SOCKETCAN_BACKEND", "Backend SocketCAN inicializado en " + config_.if0);
    return true;
}

inline bool SocketCanBackend::start() {
    if (running_.load()) {
        return true;
    }

    if (socket_fd_ < 0) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Socket no inicializado");
        return false;
    }

    // Verificar que la interfaz está up
    if (!is_interface_up()) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Interfaz " + config_.if0 + " no está activa");
        return false;
    }

    // Iniciar thread de recepción
    thread_running_ = true;
    receive_thread_ = std::thread(&SocketCanBackend::receive_loop, this);

    running_ = true;
    LOG_INFO("SOCKETCAN_BACKEND", "Backend SocketCAN iniciado");
    return true;
}

inline void SocketCanBackend::stop() {
    if (!running_.load()) {
        return;
    }

    running_ = false;
    thread_running_ = false;

    if (receive_thread_.joinable()) {
        receive_thread_.join();
    }

    close_socket(socket_fd_);
    close_socket(socket_fd_if1_);

    LOG_INFO("SOCKETCAN_BACKEND", "Backend SocketCAN detenido");
}

inline bool SocketCanBackend::is_running() const {
    return running_.load();
}

inline bool SocketCanBackend::send(const CanFrame& frame) {
    if (!running_.load() || socket_fd_ < 0) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Backend no está corriendo");
        return false;
    }

    struct can_frame can_msg;
    std::memset(&can_msg, 0, sizeof(can_msg));

    can_msg.can_id = frame.id;
    if (frame.extended) {
        can_msg.can_id |= CAN_EFF_FLAG;
    }

    can_msg.can_dlc = static_cast<uint8_t>(frame.payload.size());
    if (can_msg.can_dlc > 8) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Payload demasiado grande");
        return false;
    }

    std::memcpy(can_msg.data, frame.payload.data(), can_msg.can_dlc);

    // Enviar por socket primario
    ssize_t nbytes = write(socket_fd_, &can_msg, sizeof(can_msg));

    if (nbytes != sizeof(can_msg)) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.tx_errors++;
        LOG_ERROR("SOCKETCAN_BACKEND", "Error TX: " + std::string(strerror(errno)));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.tx_count++;
        stats_.last_tx_timestamp_us = get_current_time_us();
    }

    LOG_DEBUG("SOCKETCAN_BACKEND", "TX: ID=0x" + 
              std::to_string(frame.id) + 
              " dlc=" + std::to_string(can_msg.can_dlc));

    return true;
}

inline std::optional<CanFrame> SocketCanBackend::receive() {
    std::lock_guard<std::mutex> lock(rx_mutex_);
    
    if (rx_queue_.empty()) {
        return std::nullopt;
    }

    auto frame = rx_queue_.front();
    rx_queue_.pop();
    return frame;
}

inline std::vector<CanFrame> SocketCanBackend::receive_all() {
    std::lock_guard<std::mutex> lock(rx_mutex_);
    
    std::vector<CanFrame> frames;
    while (!rx_queue_.empty()) {
        frames.push_back(rx_queue_.front());
        rx_queue_.pop();
    }
    return frames;
}

inline bool SocketCanBackend::set_filter(uint32_t can_id, uint32_t can_mask) {
    struct can_filter filter;
    filter.can_id = can_id;
    filter.can_mask = can_mask;

    if (setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter)) < 0) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Error configurando filtro: " + 
                 std::string(strerror(errno)));
        return false;
    }

    // Aplicar también a interfaz secundaria
    if (socket_fd_if1_ >= 0) {
        setsockopt(socket_fd_if1_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));
    }

    filters_.clear();
    filters_.push_back(filter);
    filter_all_ = false;

    LOG_INFO("SOCKETCAN_BACKEND", "Filtro configurado: ID=0x" + 
             std::to_string(can_id) + " Mask=0x" + std::to_string(can_mask));
    return true;
}

inline void SocketCanBackend::clear_filters() {
    // Desactivar todos los filtros (recibir todo)
    struct can_filter filter;
    filter.can_id = 0;
    filter.can_mask = 0;

    setsockopt(socket_fd_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));
    
    if (socket_fd_if1_ >= 0) {
        setsockopt(socket_fd_if1_, SOL_CAN_RAW, CAN_RAW_FILTER, &filter, sizeof(filter));
    }

    filters_.clear();
    filter_all_ = true;

    LOG_INFO("SOCKETCAN_BACKEND", "Filtros limpiados - modo promiscuo");
}

inline BackendStats SocketCanBackend::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

inline uint64_t SocketCanBackend::get_time_us() const {
    return get_current_time_us();
}

// ─────────────────────────────────────────────────────────────────────
// Métodos específicos
// ─────────────────────────────────────────────────────────────────────

inline bool SocketCanBackend::set_secondary_interface(const std::string& ifname) {
    if (running_.load()) {
        LOG_ERROR("SOCKETCAN_BACKEND", "No se puede cambiar interfaz mientras está corriendo");
        return false;
    }

    config_.if1 = ifname;
    return open_socket(config_.if1, socket_fd_if1_);
}

inline SocketCanBackend::ExtendedErrorStats SocketCanBackend::get_extended_error_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return ext_error_stats_;
}

inline bool SocketCanBackend::is_interface_up() const {
    if (socket_fd_ < 0) return false;

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, config_.if0.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(socket_fd_, SIOCGIFFLAGS, &ifr) < 0) {
        return false;
    }

    return (ifr.ifr_flags & IFF_UP) && (ifr.ifr_flags & IFF_RUNNING);
}

inline bool SocketCanBackend::reset_interface() {
    stop();
    
    // Cerrar sockets
    close_socket(socket_fd_);
    close_socket(socket_fd_if1_);
    
    socket_fd_ = -1;
    socket_fd_if1_ = -1;
    
    // Re-inicializar
    return initialize() && start();
}

inline void SocketCanBackend::set_error_callback(ErrorCallback cb) {
    error_callback_ = std::move(cb);
}

// ─────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────

inline bool SocketCanBackend::open_socket(const std::string& ifname, int& fd) {
    // Crear socket
    fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Error creando socket: " + 
                  std::string(strerror(errno)));
        return false;
    }

    // Configurar modo no bloqueante
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    // Obtener índice de interfaz
    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, ifname.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Interfaz no encontrada: " + ifname);
        close(fd);
        fd = -1;
        return false;
    }

    // Bind a la interfaz
    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("SOCKETCAN_BACKEND", "Error bind: " + std::string(strerror(errno)));
        close(fd);
        fd = -1;
        return false;
    }

    // Configurar timeout de recepción
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 100ms timeout
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    LOG_INFO("SOCKETCAN_BACKEND", "Socket abierto para " + ifname);
    return true;
}

inline void SocketCanBackend::close_socket(int& fd) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

inline void SocketCanBackend::receive_loop() {
    fd_set read_fds;
    int max_fd = std::max(socket_fd_, socket_fd_if1_);

    while (thread_running_.load()) {
        FD_ZERO(&read_fds);
        
        if (socket_fd_ >= 0) {
            FD_SET(socket_fd_, &read_fds);
        }
        if (socket_fd_if1_ >= 0) {
            FD_SET(socket_fd_if1_, &read_fds);
        }

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = config_.loopback_ms * 1000;

        int ret = select(max_fd + 1, &read_fds, nullptr, nullptr, &tv);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG_ERROR("SOCKETCAN_BACKEND", "Error en select: " + std::string(strerror(errno)));
            break;
        }

        if (ret == 0) continue;  // Timeout

        // Procesar socket primario
        if (socket_fd_ >= 0 && FD_ISSET(socket_fd_, &read_fds)) {
            struct can_frame frame;
            ssize_t nbytes = read(socket_fd_, &frame, sizeof(frame));
            
            if (nbytes == sizeof(frame)) {
                process_can_frame(frame);
            } else if (nbytes < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.rx_errors++;
            }
        }

        // Procesar socket secundario
        if (socket_fd_if1_ >= 0 && FD_ISSET(socket_fd_if1_, &read_fds)) {
            struct can_frame frame;
            ssize_t nbytes = read(socket_fd_if1_, &frame, sizeof(frame));
            
            if (nbytes == sizeof(frame)) {
                process_can_frame(frame);
            }
        }
    }
}

inline bool SocketCanBackend::process_can_frame(const struct can_frame& frame) {
    // Verificar si el ID pasa los filtros
    if (!filters_.empty() && !filter_all_) {
        bool passes = false;
        for (const auto& f : filters_) {
            if ((frame.can_id & f.can_mask) == (f.can_id & f.can_mask)) {
                passes = true;
                break;
            }
        }
        if (!passes) return false;
    }

    auto can_frame = convert_to_frame(frame);

    {
        std::lock_guard<std::mutex> lock(rx_mutex_);
        rx_queue_.push(can_frame);
    }

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.rx_count++;
        stats_.last_rx_timestamp_us = get_current_time_us();
    }

    // Notificar callback
    if (can_callback_) {
        can_callback_(can_frame);
    }

    LOG_DEBUG("SOCKETCAN_BACKEND", "RX: ID=0x" + 
              std::to_string(frame.can_id) + 
              " dlc=" + std::to_string(frame.can_dlc));

    return true;
}

inline CanFrame SocketCanBackend::convert_to_frame(const struct can_frame& can_msg) {
    CanFrame frame;
    frame.id = can_msg.can_id & CAN_EFF_MASK;
    frame.extended = (can_msg.can_id & CAN_EFF_FLAG) != 0;
    frame.payload.assign(can_msg.data, can_msg.data + can_msg.can_dlc);
    frame.timestamp_us = get_current_time_us();
    frame.valid = true;
    return frame;
}

inline uint64_t SocketCanBackend::get_current_time_us() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

} // namespace testing
} // namespace ecu
