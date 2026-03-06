#include "error_publisher.hpp"
#include "logging.hpp"
#include <cstring>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace ecu {

// Instancia global
ErrorPublisher g_error_publisher;

ErrorPublisher::ErrorPublisher() 
    : server_fd_(-1), running_(false), initialized_(false) {
}

ErrorPublisher::~ErrorPublisher() {
    stop();
}

bool ErrorPublisher::init(const ErrorPublisherConfig& config) {
    config_ = config;
    
    // Crear directorio si no existe
    std::string dir = config_.socket_path.substr(0, config_.socket_path.rfind('/'));
    if (!dir.empty()) {
        system(("mkdir -p " + dir).c_str());
    }
    
    // Eliminar socket anterior si existe
    unlink(config_.socket_path.c_str());
    
    // Crear socket
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        LOG_ERROR("ERROR_PUB", "Failed to create socket");
        return false;
    }
    
    // Configurar dirección
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, config_.socket_path.c_str(), sizeof(addr.sun_path) - 1);
    
    // Bind
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("ERROR_PUB", "Failed to bind socket: " + config_.socket_path);
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    // Listen
    if (listen(server_fd_, 5) < 0) {
        LOG_ERROR("ERROR_PUB", "Failed to listen");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    // Hacer socket no bloqueante para accept
    int flags = fcntl(server_fd_, F_GETFL, 0);
    fcntl(server_fd_, F_SETFL, flags | O_NONBLOCK);
    
    initialized_.store(true);
    LOG_INFO("ERROR_PUB", "Publisher initialized: " + config_.socket_path);
    return true;
}

void ErrorPublisher::start() {
    if (!initialized_.load()) {
        LOG_ERROR("ERROR_PUB", "Publisher not initialized");
        return;
    }
    
    running_.store(true);
    
    // Iniciar hilo accept
    accept_thread_ = std::thread([this]() {
        accept_loop();
    });
    
    // Iniciar hilo publish
    publish_thread_ = std::thread([this]() {
        publisher_thread();
    });
    
    LOG_INFO("ERROR_PUB", "Publisher started");
}

void ErrorPublisher::stop() {
    running_.store(false);
    
    // Cerrar todos los clientes
    {
        std::lock_guard<std::mutex> lock(clients_mutex_);
        for (int client : clients_) {
            close(client);
        }
        clients_.clear();
    }
    
    // Cerrar servidor
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    
    // Esperar hilos
    if (accept_thread_.joinable()) accept_thread_.join();
    if (publish_thread_.joinable()) publish_thread_.join();
    
    // Eliminar socket
    unlink(config_.socket_path.c_str());
    
    initialized_.store(false);
    LOG_INFO("ERROR_PUB", "Publisher stopped");
}

void ErrorPublisher::accept_loop() {
    while (running_.load()) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client = accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
        
        if (client >= 0) {
            // Añadir cliente
            {
                std::lock_guard<std::mutex> lock(clients_mutex_);
                clients_.push_back(client);
            }
            
            LOG_INFO("ERROR_PUB", "Client connected");
            
            // Enviar snapshot si configurado
            if (config_.send_snap_on_connect) {
                ErrorSnapshot snap = generate_snapshot();
                std::string json = snapshot_to_json(snap);
                send(client, json.c_str(), json.size(), 0);
            }
        } else {
            // No hay conexión, esperar un poco
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void ErrorPublisher::publisher_thread() {
    while (running_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(config_.snap_interval_ms));
        
        // Generar y enviar snapshot periódicamente
        ErrorSnapshot snap = generate_snapshot();
        std::string json = "{\"type\":\"SNAPSHOT\"," + snapshot_to_json(snap).substr(1);
        broadcast(json);
    }
}

void ErrorPublisher::publish_event(const ErrorEvent& event) {
    std::string json = event_to_json(event);
    broadcast(json);
}

void ErrorPublisher::publish_snapshot(const ErrorSnapshot& snap) {
    std::string json = snapshot_to_json(snap);
    broadcast(json);
}

void ErrorPublisher::broadcast(const std::string& data) {
    std::lock_guard<std::mutex> lock(clients_mutex_);
    
    auto it = clients_.begin();
    while (it != clients_.end()) {
        ssize_t sent = send(*it, data.c_str(), data.size(), 0);
        if (sent < 0) {
            // Cliente desconectado
            close(*it);
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

ErrorSnapshot ErrorPublisher::generate_snapshot() const {
    ErrorSnapshot snap;
    snap.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Por implementar: contar errores del ErrorManager
    snap.total_errors = 0;
    snap.active_errors = 0;
    snap.critical_count = 0;
    snap.grave_count = 0;
    snap.leve_count = 0;
    snap.informative_count = 0;
    
    return snap;
}

std::string ErrorPublisher::event_to_json(const ErrorEvent& evt) const {
    std::ostringstream oss;
    oss << "{\"type\":\"EVENT\","
        << "\"ts\":" << evt.timestamp_ms << ","
        << "\"code\":" << static_cast<uint16_t>(evt.code) << ","
        << "\"level\":\"" << error_level_to_string(evt.level) << "\","
        << "\"group\":\"" << error_group_to_string(evt.group) << "\","
        << "\"status\":" << static_cast<uint8_t>(evt.status) << ","
        << "\"origin\":\"" << evt.origin << "\","
        << "\"desc\":\"" << evt.description << "\","
        << "\"count\":" << evt.count << "}";
    return oss.str();
}

std::string ErrorPublisher::snapshot_to_json(const ErrorSnapshot& snap) const {
    std::ostringstream oss;
    oss << "{\"type\":\"SNAPSHOT\","
        << "\"ts\":" << snap.timestamp_ms << ","
        << "\"total\":" << snap.total_errors << ","
        << "\"active\":" << snap.active_errors << ","
        << "\"crit\":" << snap.critical_count << ","
        << "\"grave\":" << snap.grave_count << ","
        << "\"leve\":" << snap.leve_count << ","
        << "\"info\":" << snap.informative_count << "}";
    return oss.str();
}

} // namespace ecu
