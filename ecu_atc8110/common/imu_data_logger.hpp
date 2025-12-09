#pragma once

#include "csv_logger_base.hpp"
#include "types.hpp"
#include <sstream>

namespace common {

/**
 * Logger para datos de IMU (Inertial Measurement Unit)
 * Registra acelerómetro, giroscopio y ángulos de Euler
 */
class ImuDataLogger : public CsvLoggerBase {
public:
    explicit ImuDataLogger(const fs::path& session_directory);
    
    // Registra datos de IMU
    void log_imu_data(const ImuData& data);

protected:
    std::string get_csv_header() const override;

private:
    std::string imu_data_to_csv(const ImuData& data) const;
};

} // namespace common
