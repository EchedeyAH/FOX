#pragma once

#include "csv_logger_base.hpp"
#include "types.hpp"
#include <sstream>

namespace common {

/**
 * Logger para datos de los 4 motores
 * Registra RPM, torque, temperaturas y estado de cada motor
 */
class MotorDataLogger : public CsvLoggerBase {
public:
    explicit MotorDataLogger(const std::filesystem::path& session_directory);
    
    // Registra el estado de un motor específico
    void log_motor_state(int motor_id, const MotorState& state);
    
    // Registra el estado de todos los motores
    void log_all_motors(const std::array<MotorState, 4>& motors);

protected:
    std::string get_csv_header() const override;

private:
    std::string motor_state_to_csv(int motor_id, const MotorState& state) const;
};

} // namespace common
