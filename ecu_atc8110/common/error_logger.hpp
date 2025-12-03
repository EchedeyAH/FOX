#pragma once

#include "csv_logger_base.hpp"
#include <sstream>
#include <string>

namespace common {

/**
 * Logger especializado para errores y eventos del sistema
 * Registra todos los errores, warnings y eventos críticos
 */
class ErrorLogger : public CsvLoggerBase {
public:
    enum class Severity {
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    
    explicit ErrorLogger(const std::filesystem::path& session_directory);
    
    // Registra un error o evento
    void log_error(Severity severity,
                  const std::string& subsystem,
                  int error_code,
                  const std::string& message,
                  const std::string& context = "");
    
    // Métodos de conveniencia
    void log_info(const std::string& subsystem, const std::string& message);
    void log_warning(const std::string& subsystem, int code, const std::string& message);
    void log_error(const std::string& subsystem, int code, const std::string& message);
    void log_critical(const std::string& subsystem, int code, const std::string& message);

protected:
    std::string get_csv_header() const override;

private:
    std::string severity_to_string(Severity severity) const;
    
    std::string error_to_csv(Severity severity,
                            const std::string& subsystem,
                            int error_code,
                            const std::string& message,
                            const std::string& context) const;
};

} // namespace common
