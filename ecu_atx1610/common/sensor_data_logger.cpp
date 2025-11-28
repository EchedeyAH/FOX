#include "sensor_data_logger.hpp"

namespace common {

SensorDataLogger::SensorDataLogger(const std::filesystem::path& session_directory)
    : CsvLoggerBase({session_directory, "sensors", 1000, 100, 60}) {
}

void SensorDataLogger::log_vehicle_state(const VehicleState& state) {
    write_line(vehicle_state_to_csv(state));
}

std::string SensorDataLogger::get_csv_header() const {
    return "timestamp,accelerator,brake,steering,suspension_fl,suspension_fr,suspension_rl,suspension_rr,reverse";
}

std::string SensorDataLogger::vehicle_state_to_csv(const VehicleState& state) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << state.accelerator << ",";
    csv << state.brake << ",";
    csv << state.steering << ",";
    csv << state.suspension_mm[0] << ",";  // Front Left
    csv << state.suspension_mm[1] << ",";  // Front Right
    csv << state.suspension_mm[2] << ",";  // Rear Left
    csv << state.suspension_mm[3] << ",";  // Rear Right
    csv << (state.reverse ? 1 : 0);
    
    return csv.str();
}

} // namespace common
