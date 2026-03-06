#pragma once
/**
 * error_publisher.hpp
 * Publisher de errores via UNIX Domain Socket para HMI
 * Socket: /run/ecu/errors.sock
 * 
 * Formato: JSON line-delimited con proto_version
 * Al conectar: enviar SNAPSHOT completo
 * En cambios: enviar EVENT
 */

#include <cstdint>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "error_catalog.hpp"

namespace ecu {

// Estados de error
enum class ErrorStatus : uint8_t {
    ACTIVO = 0,
    RESUELTO = 1,
    ACK = 2
};

// Evento de error para HMI
struct ErrorEvent {
    uint64_t timestamp_ms;      // Timestamp del evento
    ErrorCode code;             // Código de error
    ErrorLevel level;           // Nivel del error
    ErrorGroup group;           // Grupo del error
    ErrorStatus status;         // Estado ACTIVO/RESUELTO/ACK
    const char* origin;         // Origen: "M1", "BMS", "SUP", etc.
    const char* description;    // Descripción para HMI
    uint32_t count;             // Número de ocurrencias
};

// Snapshot del sistema de errores
struct ErrorSnapshot {
    uint64_t timestamp_ms;
    uint32_t total_errors;
    uint32_t active_errors;
    uint32_t critical_count;
    uint32_t grave_count;
    uint32_t leve_count;
    uint32_t informative_count;
    std::vector<ErrorEvent> active_events;
};

// Configuración del publisher
struct ErrorPublisherConfig {
    std::string socket_path = "/run/ecu/errors.sock";
    uint32_t snap_interval_ms = 1000;  // Intervalo para snapshots periódicos
    bool send_snap_on_connect = true;   // Enviar snapshot al conectar cliente
};

// Clase principal del publisher
class ErrorPublisher {
public:
    ErrorPublisher();
    ~ErrorPublisher();

    // Inicializar publisher
    bool init(const ErrorPublisherConfig& config = ErrorPublisherConfig());

    // Publicar evento de error
    void publish_event(const ErrorEvent& event);

    // Publicar snapshot completo
    void publish_snapshot(const ErrorSnapshot& snap);

    // Generar snapshot del estado actual
    ErrorSnapshot generate_snapshot() const;

    // Iniciar hilo del publisher
    void start();

    // Detener hilo del publisher
    void stop();

    // Verificar si está corriendo
    bool is_running() const { return running_.load(); }

private:
    // Hilo del publisher
    void publisher_thread();

    // Aceptar conexiones de clientes
    void accept_loop();

    // Enviar datos a todos los clientes conectados
    void broadcast(const std::string& data);

    // Crear JSON de evento
    std::string event_to_json(const ErrorEvent& evt) const;

    // Crear JSON de snapshot
    std::string snapshot_to_json(const ErrorSnapshot& snap) const;

    // Socket del servidor
    int server_fd_;
    
    // Configuración
    ErrorPublisherConfig config_;
    
    // Estado
    std::atomic<bool> running_;
    std::atomic<bool> initialized_;
    
    // Hilos
    std::thread accept_thread_;
    std::thread publish_thread_;
    
    // Clientes conectados
    std::vector<int> clients_;
    mutable std::mutex clients_mutex_;
    
    // Último snapshot enviado
    ErrorSnapshot last_snapshot_;
    mutable std::mutex snap_mutex_;
};

// Instancia global
extern ErrorPublisher g_error_publisher;

} // namespace ecu
