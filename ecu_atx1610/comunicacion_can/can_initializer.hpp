#pragma once

#include "../comunicacion_can/can_manager.hpp"
#include "../common/logging.hpp"
#include <memory>
#include <chrono>
#include <thread>

namespace comunicacion_can {

/**
 * Inicializador de motores y supervisor
 * Envía la secuencia de activación necesaria para que los controladores
 * de motor y el supervisor comiencen a responder
 */
class CanInitializer {
public:
    explicit CanInitializer(CanManager& can_manager) 
        : can_manager_(can_manager) {}
    
    /**
     * Inicializa todos los motores enviando comandos de activación
     * @return true si al menos un motor respondió
     */
    bool initialize_motors() {
        LOG_INFO("CanInitializer", "=== Iniciando secuencia de activación de motores ===");
        
        bool any_motor_responded = false;
        
        for (uint8_t motor_id = 1; motor_id <= 4; ++motor_id) {
            LOG_INFO("CanInitializer", "Activando motor " + std::to_string(motor_id) + "...");
            
            // Paso 1: Enviar comando inicial (throttle=0) para despertar el controlador
            if (!can_manager_.send_motor_command(motor_id, 0, 0)) {
                LOG_WARN("CanInitializer", "No se pudo enviar comando inicial a motor " + 
                        std::to_string(motor_id));
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Paso 2: Solicitar información del módulo (modelo)
            if (!can_manager_.request_motor_telemetry(motor_id, MSG_TIPO_01)) {
                LOG_WARN("CanInitializer", "No se pudo solicitar info de motor " + 
                        std::to_string(motor_id));
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Paso 3: Solicitar versión de software
            if (!can_manager_.request_motor_telemetry(motor_id, MSG_TIPO_02)) {
                LOG_WARN("CanInitializer", "No se pudo solicitar versión SW de motor " + 
                        std::to_string(motor_id));
                continue;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            
            // Paso 4: Activar switches (acelerador, freno, reversa)
            can_manager_.request_motor_telemetry(motor_id, MSG_TIPO_11); // Switch acelerador
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            
            can_manager_.request_motor_telemetry(motor_id, MSG_TIPO_12); // Switch freno
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            
            can_manager_.request_motor_telemetry(motor_id, MSG_TIPO_13); // Switch reversa
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            
            LOG_INFO("CanInitializer", "Motor " + std::to_string(motor_id) + " activado");
            any_motor_responded = true;
        }
        
        if (any_motor_responded) {
            LOG_INFO("CanInitializer", "=== Motores activados correctamente ===");
        } else {
            LOG_ERROR("CanInitializer", "=== NINGÚN MOTOR RESPONDIÓ ===");
        }
        
        return any_motor_responded;
    }
    
    /**
     * Inicializa el supervisor enviando heartbeat inicial
     * @return true si el supervisor está activo
     */
    bool initialize_supervisor() {
        LOG_INFO("CanInitializer", "=== Iniciando comunicación con supervisor ===");
        
        // Enviar heartbeat inicial para anunciar presencia de la ECU
        for (int i = 0; i < 3; ++i) {
            can_manager_.publish_heartbeat();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        LOG_INFO("CanInitializer", "=== Heartbeat enviado al supervisor ===");
        return true;
    }
    
    /**
     * Secuencia completa de inicialización CAN
     * Activa motores y supervisor en el orden correcto
     */
    bool initialize_all() {
        LOG_INFO("CanInitializer", "╔════════════════════════════════════════╗");
        LOG_INFO("CanInitializer", "║  INICIALIZACIÓN CAN - MOTORES Y SUP   ║");
        LOG_INFO("CanInitializer", "╚════════════════════════════════════════╝");
        
        // Paso 1: Inicializar supervisor primero
        bool supervisor_ok = initialize_supervisor();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Paso 2: Inicializar motores
        bool motors_ok = initialize_motors();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Paso 3: Enviar heartbeat final
        can_manager_.publish_heartbeat();
        
        if (motors_ok || supervisor_ok) {
            LOG_INFO("CanInitializer", "╔════════════════════════════════════════╗");
            LOG_INFO("CanInitializer", "║  INICIALIZACIÓN CAN COMPLETADA ✓      ║");
            LOG_INFO("CanInitializer", "╚════════════════════════════════════════╝");
            return true;
        } else {
            LOG_ERROR("CanInitializer", "╔════════════════════════════════════════╗");
            LOG_ERROR("CanInitializer", "║  ERROR EN INICIALIZACIÓN CAN ✗        ║");
            LOG_ERROR("CanInitializer", "╚════════════════════════════════════════╝");
            return false;
        }
    }

private:
    CanManager& can_manager_;
};

} // namespace comunicacion_can
