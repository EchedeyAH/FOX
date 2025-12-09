#include "session_manager.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace common {

// SessionInfo implementation
int64_t SessionInfo::get_duration_seconds() const {
    if (!end_time.has_value()) {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
    }
    return std::chrono::duration_cast<std::chrono::seconds>(end_time.value() - start_time).count();
}

std::string SessionInfo::to_json() const {
    std::ostringstream json;
    
    auto to_iso8601 = [](const std::chrono::system_clock::time_point& tp) -> std::string {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::tm tm = *std::localtime(&time_t);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
        return oss.str();
    };
    
    json << "{\n";
    json << "  \"pilot_name\": \"" << pilot_name << "\",\n";
    json << "  \"session_id\": \"" << session_id << "\",\n";
    json << "  \"start_time\": \"" << to_iso8601(start_time) << "\",\n";
    
    if (end_time.has_value()) {
        json << "  \"end_time\": \"" << to_iso8601(end_time.value()) << "\",\n";
        json << "  \"duration_seconds\": " << get_duration_seconds() << ",\n";
    } else {
        json << "  \"end_time\": null,\n";
        json << "  \"duration_seconds\": " << get_duration_seconds() << ",\n";
    }
    
    json << "  \"notes\": \"" << notes << "\",\n";
    json << "  \"vehicle_id\": \"" << vehicle_id << "\",\n";
    json << "  \"conditions\": \"" << conditions << "\"\n";
    json << "}";
    
    return json.str();
}

// SessionManager implementation
SessionManager::SessionManager(const std::string& base_directory)
    : base_directory_(base_directory) {
    // Crear directorio base si no existe
    fs::create_directories(base_directory_);
}

SessionManager::~SessionManager() {
    if (is_session_active()) {
        end_session();
    }
}

bool SessionManager::start_session(const std::string& pilot_name, 
                                   const std::string& notes, 
                                   const std::string& conditions) {
    if (is_session_active()) {
        std::cerr << "Error: Ya hay una sesión activa. Finalice la sesión actual primero.\n";
        return false;
    }
    
    if (pilot_name.empty()) {
        std::cerr << "Error: El nombre del piloto no puede estar vacío.\n";
        return false;
    }
    
    SessionInfo session;
    session.pilot_name = pilot_name;
    session.session_id = generate_session_id();  // Solo fecha: YYYY-MM-DD
    session.start_time = std::chrono::system_clock::now();
    session.notes = notes;
    session.conditions = conditions;
    
    // Crear estructura de directorios
    std::string pilot_folder = "pilot_" + sanitize_pilot_name(pilot_name);
    std::string session_folder = "session_" + session.session_id;
    session.session_directory = base_directory_ / pilot_folder / session_folder;
    
    // Si el directorio ya existe (misma fecha), lo reutilizamos
    // Los archivos CSV se rotarán automáticamente por hora
    if (!fs::exists(session.session_directory)) {
        if (!create_session_directories(session.session_directory)) {
            std::cerr << "Error: No se pudo crear la estructura de directorios.\n";
            return false;
        }
        std::cout << "Nueva sesión creada para piloto: " << pilot_name << "\n";
    } else {
        std::cout << "Continuando sesión existente para piloto: " << pilot_name << " (fecha: " << session.session_id << ")\n";
    }
    
    current_session_ = session;
    
    std::cout << "Directorio de sesión: " << session.session_directory << "\n";
    
    return true;
}

bool SessionManager::end_session() {
    if (!is_session_active()) {
        std::cerr << "Error: No hay sesión activa para finalizar.\n";
        return false;
    }
    
    current_session_->end_time = std::chrono::system_clock::now();
    
    if (!save_session_metadata(*current_session_)) {
        std::cerr << "Advertencia: No se pudo guardar la metadata de la sesión.\n";
    }
    
    std::cout << "Sesión finalizada. Duración: " 
              << current_session_->get_duration_seconds() << " segundos\n";
    
    current_session_.reset();
    return true;
}

std::optional<SessionInfo> SessionManager::get_current_session() const {
    return current_session_;
}

bool SessionManager::is_session_active() const {
    return current_session_.has_value();
}

fs::path SessionManager::get_session_directory() const {
    if (current_session_.has_value()) {
        return current_session_->session_directory;
    }
    return "";
}

std::vector<SessionInfo> SessionManager::list_pilot_sessions(const std::string& pilot_name) const {
    std::vector<SessionInfo> sessions;
    std::string pilot_folder = "pilot_" + sanitize_pilot_name(pilot_name);
    fs::path pilot_path = base_directory_ / pilot_folder;
    
    if (!fs::exists(pilot_path)) {
        return sessions;
    }
    
    for (const auto& entry : fs::directory_iterator(pilot_path)) {
        if (entry.is_directory()) {
            // Intentar cargar session_info.json
            fs::path metadata_file = entry.path() / "session_info.json";
            if (fs::exists(metadata_file)) {
                // Aquí se podría implementar parseo JSON completo
                // Por ahora solo agregamos info básica
                SessionInfo info;
                info.pilot_name = pilot_name;
                info.session_directory = entry.path();
                sessions.push_back(info);
            }
        }
    }
    
    return sessions;
}

bool SessionManager::create_session_directories(const fs::path& session_path) {
    try {
        // Crear directorio principal de sesión
        fs::create_directories(session_path);
        
        // Crear subdirectorios para cada subsistema
        std::vector<std::string> subsystems = {
            "bms", "motors", "sensors", "imu", "system", "can", "errors"
        };
        
        for (const auto& subsystem : subsystems) {
            fs::create_directories(session_path / subsystem);
        }
        
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error al crear directorios: " << e.what() << "\n";
        return false;
    }
}

bool SessionManager::save_session_metadata(const SessionInfo& session) {
    try {
        fs::path metadata_file = session.session_directory / "session_info.json";
        std::ofstream file(metadata_file);
        
        if (!file.is_open()) {
            return false;
        }
        
        file << session.to_json();
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error al guardar metadata: " << e.what() << "\n";
        return false;
    }
}

std::string SessionManager::sanitize_pilot_name(const std::string& pilot_name) const {
    std::string sanitized = pilot_name;
    
    // Convertir a minúsculas
    std::transform(sanitized.begin(), sanitized.end(), sanitized.begin(), ::tolower);
    
    // Reemplazar espacios y caracteres especiales con guiones bajos
    std::replace_if(sanitized.begin(), sanitized.end(), 
                   [](char c) { return !std::isalnum(c); }, '_');
    
    return sanitized;
}

std::string SessionManager::generate_session_id() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    // Solo fecha, sin hora - todas las sesiones del mismo día van a la misma carpeta
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

} // namespace common
