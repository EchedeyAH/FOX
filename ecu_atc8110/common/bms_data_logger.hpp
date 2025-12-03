#pragma once

#include "csv_logger_base.hpp"
#include "types.hpp"
#include <sstream>

namespace common {

/**
 * Logger especializado para datos del BMS (Battery Management System)
 * Registra voltajes de celdas, temperaturas, estado del pack y alarmas
 */
class BmsDataLogger : public CsvLoggerBase {
public:
    explicit BmsDataLogger(const std::filesystem::path& session_directory);
    
    // Registra el estado completo de la batería
    void log_battery_state(const BatteryState& state);

protected:
    std::string get_csv_header() const override;

private:
    std::string battery_state_to_csv(const BatteryState& state) const;
};

} // namespace common
