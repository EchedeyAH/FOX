#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

namespace common {

struct SessionInfo {
    std::string pilot_name;
    std::string session_id;
    std::chrono::system_clock::time_point start_time;
    std::optional<std::chrono::system_clock::time_point> end_time;
    std::filesystem::path session_directory;
    std::string notes;
    std::string conditions;
    std::string vehicle_id{"FOX-001"};
    
    // Calcula la duración de la sesión en segundos
    int64_t get_duration_seconds() const;
    
    // Convierte a formato JSON para guardar metadata
    std::string to_json() const;
};

class SessionManager {
public:
    SessionManager(const std::string& base_directory = "/var/log/ecu_atx1610");
    ~SessionManager();
    
    // Crea una nueva sesión con el nombre del piloto
    bool start_session(const std::string& pilot_name, 
                      const std::string& notes = "", 
                      const std::string& conditions = "");
    
    // Finaliza la sesión actual y guarda metadata
    bool end_session();
    
    // Obtiene información de la sesión actual
    std::optional<SessionInfo> get_current_session() const;
    
    // Verifica si hay una sesión activa
    bool is_session_active() const;
    
    // Obtiene el directorio de la sesión actual
    std::filesystem::path get_session_directory() const;
    
    // Lista todas las sesiones de un piloto
    std::vector<SessionInfo> list_pilot_sessions(const std::string& pilot_name) const;

private:
    std::filesystem::path base_directory_;
    std::optional<SessionInfo> current_session_;
    
    // Crea la estructura de directorios para la sesión
    bool create_session_directories(const std::filesystem::path& session_path);
    
    // Guarda el archivo session_info.json
    bool save_session_metadata(const SessionInfo& session);
    
    // Genera un nombre de carpeta seguro a partir del nombre del piloto
    std::string sanitize_pilot_name(const std::string& pilot_name) const;
    
    // Genera el ID de sesión basado en timestamp
    std::string generate_session_id() const;
};

} // namespace common
