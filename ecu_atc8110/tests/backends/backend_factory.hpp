#pragma once
/**
 * backend_factory.hpp
 * Factory para crear instancias de backends CAN
 * 
 * Uso:
 *   BackendConfig config;
 *   config.backend_type = "sim";  // o "socketcan"
 *   auto backend = create_can_backend(config);
 */

#include "ican_backend.hpp"
#include "sim_backend.hpp"
#include "socketcan_backend.hpp"

#include <memory>
#include <stdexcept>

namespace ecu {
namespace testing {

// ============================================================================
// FACTORY IMPLEMENTATION
// ============================================================================

inline std::unique_ptr<ICanBackend> create_can_backend(const BackendConfig& config) {
    if (config.backend_type == "sim" || config.backend_type == "simulator") {
        return std::make_unique<SimBackend>(config);
    }
    else if (config.backend_type == "socketcan" || config.backend_type == "real") {
        return std::make_unique<SocketCanBackend>(config);
    }
    else {
        throw std::invalid_argument("Unknown backend type: " + config.backend_type + 
                                   ". Use 'sim' or 'socketcan'");
    }
}

// ============================================================================
// BACKEND MANAGER
// ============================================================================

/**
 * Gestor de backends para el framework de tests
 * Provee acceso centralizado al backend activo
 */
class BackendManager {
public:
    BackendManager() = default;
    ~BackendManager() = default;

    /**
     * Inicializar el backend especificado en la configuración
     */
    bool initialize(const BackendConfig& config) {
        config_ = config;
        
        try {
            backend_ = create_can_backend(config);
            
            if (!backend_->initialize()) {
                LOG_ERROR("BACKEND_MGR", "Error inicializando backend");
                return false;
            }
            
            LOG_INFO("BACKEND_MGR", "Backend " + config.backend_type + " inicializado");
            return true;
        }
        catch (const std::exception& e) {
            LOG_ERROR("BACKEND_MGR", std::string("Excepción: ") + e.what());
            return false;
        }
    }

    /**
     * Iniciar el backend
     */
    bool start() {
        if (!backend_) {
            LOG_ERROR("BACKEND_MGR", "Backend no inicializado");
            return false;
        }
        
        return backend_->start();
    }

    /**
     * Detener el backend
     */
    void stop() {
        if (backend_) {
            backend_->stop();
        }
    }

    /**
     * Obtener referencia al backend activo
     */
    ICanBackend* get() { return backend_.get(); }
    
    const ICanBackend* get() const { return backend_.get(); }

    /**
     * Obtener configuración actual
     */
    const BackendConfig& get_config() const { return config_; }

    /**
     * Verificar si está corriendo
     */
    bool is_running() const { 
        return backend_ && backend_->is_running(); 
    }

    /**
     * Obtener tipo de backend
     */
    std::string get_type() const {
        return backend_ ? backend_->get_type() : "none";
    }

    /**
     * Verificar si es simulación
     */
    bool is_simulation() const {
        return get_type() == "sim";
    }

    /**
     * Verificar si es backend real
     */
    bool is_real() const {
        return get_type() == "socketcan";
    }

private:
    BackendConfig config_;
    std::unique_ptr<ICanBackend> backend_;
};

} // namespace testing
} // namespace ecu
