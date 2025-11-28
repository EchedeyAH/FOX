#include "imu_data_logger.hpp"

namespace common {

ImuDataLogger::ImuDataLogger(const std::filesystem::path& session_directory)
    : CsvLoggerBase({session_directory, "imu", 1000, 100, 60}) {
}

void ImuDataLogger::log_imu_data(const ImuData& data) {
    write_line(imu_data_to_csv(data));
}

std::string ImuDataLogger::get_csv_header() const {
    return "timestamp,accel_x_g,accel_y_g,accel_z_g,gyro_x_rad_s,gyro_y_rad_s,gyro_z_rad_s,roll_rad,pitch_rad,yaw_rad";
}

std::string ImuDataLogger::imu_data_to_csv(const ImuData& data) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << data.accel_g[0] << ",";
    csv << data.accel_g[1] << ",";
    csv << data.accel_g[2] << ",";
    csv << data.gyro_rad_s[0] << ",";
    csv << data.gyro_rad_s[1] << ",";
    csv << data.gyro_rad_s[2] << ",";
    csv << data.euler_rad[0] << ",";  // Roll
    csv << data.euler_rad[1] << ",";  // Pitch
    csv << data.euler_rad[2];         // Yaw
    
    return csv.str();
}

} // namespace common
