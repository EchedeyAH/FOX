#include "error_logger.hpp"

namespace common {

ErrorLogger::ErrorLogger(const fs::path& session_directory)
    : CsvLoggerBase({session_directory, "errors", 500, 50, 30}) {
    // Rotación más frecuente (30 min) y archivos más pequeños (50 MB)
    // para no perder eventos críticos
}

void ErrorLogger::log_error(Severity severity,
                           const std::string& subsystem,
                           int error_code,
                           const std::string& message,
                           const std::string& context) {
    write_line(error_to_csv(severity, subsystem, error_code, message, context));
}

void ErrorLogger::log_info(const std::string& subsystem, const std::string& message) {
    log_error(Severity::INFO, subsystem, 0, message, "");
}

void ErrorLogger::log_warning(const std::string& subsystem, int code, const std::string& message) {
    log_error(Severity::WARNING, subsystem, code, message, "");
}

void ErrorLogger::log_error(const std::string& subsystem, int code, const std::string& message) {
    log_error(Severity::ERROR, subsystem, code, message, "");
}

void ErrorLogger::log_critical(const std::string& subsystem, int code, const std::string& message) {
    log_error(Severity::CRITICAL, subsystem, code, message, "");
}

std::string ErrorLogger::get_csv_header() const {
    return "timestamp,severity,subsystem,error_code,message,context";
}

std::string ErrorLogger::severity_to_string(Severity severity) const {
    switch (severity) {
        case Severity::INFO: return "INFO";
        case Severity::WARNING: return "WARNING";
        case Severity::ERROR: return "ERROR";
        case Severity::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

std::string ErrorLogger::error_to_csv(Severity severity,
                                     const std::string& subsystem,
                                     int error_code,
                                     const std::string& message,
                                     const std::string& context) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << severity_to_string(severity) << ",";
    csv << subsystem << ",";
    csv << error_code << ",";
    
    // Escapar comillas en el mensaje
    std::string escaped_message = message;
    size_t pos = 0;
    while ((pos = escaped_message.find('"', pos)) != std::string::npos) {
        escaped_message.replace(pos, 1, "\"\"");
        pos += 2;
    }
    csv << "\"" << escaped_message << "\",";
    
    // Escapar comillas en el contexto
    std::string escaped_context = context;
    pos = 0;
    while ((pos = escaped_context.find('"', pos)) != std::string::npos) {
        escaped_context.replace(pos, 1, "\"\"");
        pos += 2;
    }
    csv << "\"" << escaped_context << "\"";
    
    return csv.str();
}

} // namespace common
