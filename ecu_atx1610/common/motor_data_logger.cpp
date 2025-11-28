#include "motor_data_logger.hpp"

namespace common {

MotorDataLogger::MotorDataLogger(const std::filesystem::path& session_directory)
    : CsvLoggerBase({session_directory, "motors", 1000, 100, 60}) {
}

void MotorDataLogger::log_motor_state(int motor_id, const MotorState& state) {
    write_line(motor_state_to_csv(motor_id, state));
}

void MotorDataLogger::log_all_motors(const std::array<MotorState, 4>& motors) {
    for (int i = 0; i < 4; ++i) {
        log_motor_state(i, motors[i]);
    }
}

std::string MotorDataLogger::get_csv_header() const {
    return "timestamp,motor_id,label,rpm,torque_nm,inverter_temp_c,motor_temp_c,enabled";
}

std::string MotorDataLogger::motor_state_to_csv(int motor_id, const MotorState& state) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << motor_id << ",";
    csv << state.label << ",";
    csv << state.rpm << ",";
    csv << state.torque_nm << ",";
    csv << state.inverter_temp_c << ",";
    csv << state.motor_temp_c << ",";
    csv << (state.enabled ? 1 : 0);
    
    return csv.str();
}

} // namespace common
