#pragma once

#include "session_manager.hpp"
#include <memory>
#include <string>

namespace interfaces {

/**
 * API simple para integración con interfaz Qt/PySide
 * Esta clase proporciona una interfaz simplificada para que la GUI
 * pueda iniciar/detener sesiones de logging
 */
class PilotInterface {
public:
    PilotInterface();
    ~PilotInterface();
    
    /**
     * Inicia una nueva sesión de logging
     * @param pilot_name Nombre del piloto (obligatorio)
     * @param notes Notas opcionales sobre la sesión
     * @param conditions Condiciones de prueba (clima, pista, etc.)
     * @return true si la sesión se inició correctamente
     */
    bool start_session(const std::string& pilot_name,
                      const std::string& notes = "",
                      const std::string& conditions = "");
    
    /**
     * Finaliza la sesión actual
     * @return true si la sesión se finalizó correctamente
     */
    bool end_session();
    
    /**
     * Obtiene información de la sesión actual
     * @return SessionInfo si hay sesión activa, nullopt si no
     */
    std::optional<common::SessionInfo> get_current_session_info() const;
    
    /**
     * Verifica si hay una sesión activa
     * @return true si hay sesión activa
     */
    bool is_session_active() const;
    
    /**
     * Obtiene el directorio donde se están guardando los logs
     * @return Ruta del directorio de sesión
     */
    std::string get_session_directory() const;
    
    /**
     * Obtiene el nombre del piloto de la sesión actual
     * @return Nombre del piloto o string vacío si no hay sesión
     */
    std::string get_current_pilot_name() const;

private:
    std::unique_ptr<common::SessionManager> session_manager_;
};

} // namespace interfaces
