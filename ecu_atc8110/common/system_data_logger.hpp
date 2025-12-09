#pragma once

#include "csv_logger_base.hpp"
#include "types.hpp"
#include <sstream>

namespace common {

/**
 * Logger para datos generales del sistema
 * Registra presupuesto de potencia y fallos
 */
class SystemDataLogger : public CsvLoggerBase {
public:
    explicit SystemDataLogger(const fs::path& session_directory);
    
    // Registra presupuesto de potencia y fallos
    void log_system_snapshot(const PowerBudget& power, const FaultFlags& faults);

protected:
    std::string get_csv_header() const override;

private:
    std::string system_to_csv(const PowerBudget& power, const FaultFlags& faults) const;
};

} // namespace common
