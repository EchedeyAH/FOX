#pragma once

#include "csv_logger_base.hpp"
#include "types.hpp"
#include <sstream>

namespace common {

/**
 * Logger para sensores analógicos y estado del vehículo
 * Registra acelerador, freno, dirección y suspensión
 */
class SensorDataLogger : public CsvLoggerBase {
public:
    explicit SensorDataLogger(const std::filesystem::path& session_directory);
    
    // Registra el estado del vehículo (sensores)
    void log_vehicle_state(const VehicleState& state);

protected:
    std::string get_csv_header() const override;

private:
    std::string vehicle_state_to_csv(const VehicleState& state) const;
};

} // namespace common
