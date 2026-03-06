#pragma once
/**
 * telemetry_publisher.hpp
 * Publicador de telemetría para HMI via socket Unix
 * 
 * Publica periódicamente en: /run/ecu/telemetry.sock
 * 
 * Formato: JSON line-delimited (JSONL)
 * 
 * Ejemplo de mensaje:
 * {"ts":1234567890,"mode":"OK","power_factor":1.0,"torque_max":100.0,
 *   "vbat":380000,"soc":50.0,"temp_motors":[25,25,25,25],
 *   "hb_motors":[1,1,1,1],"jitter_us":150}
 * 
 * Frecuencia: configurable (default 100ms)
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "error_catalog.hpp"
#include "system_mode_manager.hpp"
#include "motor_timeout_detector.hpp"
#include "voltage_protection.hpp"

namespace common {

// ============================================================================
// CONSTANTES
// ============================================================================

constexpr char TELEMETRY_SOCKET_PATH[] = "/run/ecu/telemetry.sock";
constexpr int TELEMETRY_DEFAULT_PERIOD_MS = 100;
constexpr int SOCKET_BACKLOG = 5;
constexpr size_t MAX_PAYLOAD_SIZE = 512;

// ============================================================================
// ESTRUCTURAS
// ============================================================================

/**
 * Datos de telemetría del sistema
 */
struct TelemetryData {
    // Timestamp
    uint64_t timestamp_ms;
    
    // Estado del sistema
    SystemMode mode;
    double power_limit_factor;
    double torque_max_allowed;
    
    // Batería
    double vbat_mv;
    double soc_percent;
    double temp_battery_c;
    double current_ma;
    
    // Motores
    std::array<double, 4> temp_motors_c;
    std::array<bool, 4> motor_heartbeat;  // true = motor respondiendo
    std::array<double, 4> motor_rpm;
    std::array<double, 4> motor_torque_nm;
    
    // Scheduler
    int64_t jitter_us;
    uint64_t cycle_count;
    int overrun_count;
    
    // Pedales
    double accelerator;
    double brake;
};

/**
 * Publicador de telemetría
 */
class TelemetryPublisher {
public:
    TelemetryPublisher();
    ~TelemetryPublisher();
    
    // Iniciar publicador (crea socket y hilo)
    bool start();
    
    // Detener publicador
    void stop();
    
    // Actualizar datos de telemetría
    void update(const TelemetryData& data);
    
    // Actualizar datos individuales
    void update_mode(SystemMode mode, double power_factor);
    void update_battery(double vbat_mv, double soc, double temp_c, double current_ma);
    void update_motor(uint8_t idx, double temp_c, double rpm, double torque_nm, bool alive);
    void update_scheduler(int64_t jitter_us, uint64_t cycles, int overruns);
    void update_pedals(double accel, double brake);
    
    // Verificar si está corriendo
    bool is_running() const { return running_.load(); }

private:
    // Datos actuales
    TelemetryData data_;
    
    // Socket
    int socket_fd_;
    bool socket_ready_;
    
    // Control
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    
    // Thread
    std::thread publish_thread_;
    
    // Métodos internos
    void publish_loop();
    bool create_socket();
    void close_socket();
    bool publish_message();
    std::string serialize_json() const;
};

// ============================================================================
// IMPLEMENTACIÓN
// ============================================================================

inline TelemetryPublisher::TelemetryPublisher() 
    : socket_fd_(-1)
    , socket_ready_(false)
    , running_(false)
    , initialized_(false)
{
    data_ = TelemetryData{};
    data_.timestamp_ms = 0;
    data_.mode = SystemMode::OK;
    data_.power_limit_factor = 1.0;
    data_.torque_max_allowed = 100.0;
    data_.vbat_mv = 380000;
    data_.soc_percent = 50.0;
    data_.temp_battery_c = 25.0;
    data_.current_ma = 0;
    data_.temp_motors_c = {25.0, 25.0, 25.0, 25.0};
    data_.motor_heartbeat = {false, false, false, false};
    data_.motor_rpm = {0.0, 0.0, 0.0, 0.0};
    data_.motor_torque_nm = {0.0, 0.0, 0.0, 0.0};
    data_.jitter_us = 0;
    data_.cycle_count = 0;
    data_.overrun_count = 0;
    data_.accelerator = 0.0;
    data_.brake = 0.0;
}

inline TelemetryPublisher::~TelemetryPublisher() {
    stop();
}

inline bool TelemetryPublisher::start() {
    if (running_.load()) {
        return true;
    }
    
    // Crear socket
    if (!create_socket()) {
        LOG_ERROR("TELEMETRY", "No se pudo crear socket");
        return false;
    }
    
    socket_ready_ = true;
    running_ = true;
    initialized_ = true;
    
    // Iniciar hilo de publicación
    publish_thread_ = std::thread(&TelemetryPublisher::publish_loop, this);
    
    LOG_INFO("TELEMETRY", "Publicador iniciado en " + std::string(TELEMETRY_SOCKET_PATH));
    return true;
}

inline void TelemetryPublisher::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    
    if (publish_thread_.joinable()) {
        publish_thread_.join();
    }
    
    close_socket();
    socket_ready_ = false;
    
    LOG_INFO("TELEMETRY", "Publicador detenido");
}

inline void TelemetryPublisher::update(const TelemetryData& data) {
    data_ = data;
}

inline void TelemetryPublisher::update_mode(SystemMode mode, double power_factor) {
    data_.mode = mode;
    data_.power_limit_factor = power_factor;
}

inline void TelemetryPublisher::update_battery(double vbat_mv, double soc, double temp_c, double current_ma) {
    data_.vbat_mv = vbat_mv;
    data_.soc_percent = soc;
    data_.temp_battery_c = temp_c;
    data_.current_ma = current_ma;
}

inline void TelemetryPublisher::update_motor(uint8_t idx, double temp_c, double rpm, double torque_nm, bool alive) {
    if (idx < 4) {
        data_.temp_motors_c[idx] = temp_c;
        data_.motor_rpm[idx] = rpm;
        data_.motor_torque_nm[idx] = torque_nm;
        data_.motor_heartbeat[idx] = alive;
    }
}

inline void TelemetryPublisher::update_scheduler(int64_t jitter_us, uint64_t cycles, int overruns) {
    data_.jitter_us = jitter_us;
    data_.cycle_count = cycles;
    data_.overrun_count = overruns;
}

inline void TelemetryPublisher::update_pedals(double accel, double brake) {
    data_.accelerator = accel;
    data_.brake = brake;
}

inline void TelemetryPublisher::publish_loop() {
    while (running_.load()) {
        // Publicar mensaje
        if (socket_ready_) {
            publish_message();
        }
        
        // Esperar siguiente período
        std::this_thread::sleep_for(std::chrono::milliseconds(TELEMETRY_DEFAULT_PERIOD_MS));
    }
}

inline bool TelemetryPublisher::create_socket() {
    // Crear socket Unix
    socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd_ < 0) {
        return false;
    }
    
    // Configurar dirección
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, TELEMETRY_SOCKET_PATH, sizeof(addr.sun_path) - 1);
    
    // Eliminar socket existente
    unlink(TELEMETRY_SOCKET_PATH);
    
    // Bind
    if (bind(socket_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // Listen
    if (listen(socket_fd_, SOCKET_BACKLOG) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        return false;
    }
    
    // chmod
    chmod(TELEMETRY_SOCKET_PATH, 0666);
    
    return true;
}

inline void TelemetryPublisher::close_socket() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    unlink(TELEMETRY_SOCKET_PATH);
}

inline bool TelemetryPublisher::publish_message() {
    // Serializar a JSON
    std::string json = serialize_json();
    
    if (json.empty()) {
        return false;
    }
    
    // Aceptar conexión si hay alguien esperando
    int client_fd = accept(socket_fd_, nullptr, nullptr);
    if (client_fd < 0) {
        // No hay cliente conectado - no es error
        return false;
    }
    
    // Enviar datos
    ssize_t sent = send(client_fd, json.c_str(), json.length(), 0);
    close(client_fd);
    
    return (sent > 0);
}

inline std::string TelemetryPublisher::serialize_json() const {
    // Crear JSON manualmente para evitar dependencias
    char buffer[MAX_PAYLOAD_SIZE];
    
    const char* mode_str = (data_.mode == SystemMode::OK) ? "\"OK\"" :
                           (data_.mode == SystemMode::LIMP_MODE) ? "\"LIMP\"" :
                           "\"STOP\"";
    
    int len = snprintf(buffer, sizeof(buffer),
        "{"
        "\"ts\":%llu,"
        "\"mode\":%s,"
        "\"power_factor\":%.2f,"
        "\"torque_max\":%.1f,"
        "\"vbat\":%.0f,"
        "\"soc\":%.1f,"
        "\"temp_bat\":%.1f,"
        "\"current\":%.0f,"
        "\"temp_motors\":[%.1f,%.1f,%.1f,%.1f],"
        "\"hb_motors\":[%d,%d,%d,%d],"
        "\"rpm_motors\":[%.0f,%.0f,%.0f,%.0f],"
        "\"torque_motors\":[%.1f,%.1f,%.1f,%.1f],"
        "\"jitter_us\":%ld,"
        "\"cycles\":%llu,"
        "\"overruns\":%d,"
        "\"accel\":%.2f,"
        "\"brake\":%.2f"
        "}\n",
        
        (unsigned long long)data_.timestamp_ms,
        mode_str,
        data_.power_limit_factor,
        data_.torque_max_allowed,
        data_.vbat_mv,
        data_.soc_percent,
        data_.temp_battery_c,
        data_.current_ma,
        data_.temp_motors_c[0], data_.temp_motors_c[1], 
        data_.temp_motors_c[2], data_.temp_motors_c[3],
        data_.motor_heartbeat[0] ? 1 : 0,
        data_.motor_heartbeat[1] ? 1 : 0,
        data_.motor_heartbeat[2] ? 1 : 0,
        data_.motor_heartbeat[3] ? 1 : 0,
        data_.motor_rpm[0], data_.motor_rpm[1],
        data_.motor_rpm[2], data_.motor_rpm[3],
        data_.motor_torque_nm[0], data_.motor_torque_nm[1],
        data_.motor_torque_nm[2], data_.motor_torque_nm[3],
        (long)data_.jitter_us,
        (unsigned long long)data_.cycle_count,
        data_.overrun_count,
        data_.accelerator,
        data_.brake
    );
    
    if (len < 0 || (size_t)len >= sizeof(buffer)) {
        return "{}";
    }
    
    return std::string(buffer);
}

// Instancia global
inline TelemetryPublisher& get_telemetry_publisher() {
    static TelemetryPublisher publisher;
    return publisher;
}

} // namespace common
