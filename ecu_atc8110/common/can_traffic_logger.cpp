#include "can_traffic_logger.hpp"
#include <iomanip>

namespace common {

CanTrafficLogger::CanTrafficLogger(const fs::path& session_directory)
    : CsvLoggerBase({session_directory, "can", 5000, 100, 60}) {
    // Buffer más grande para CAN (5000) ya que hay mucho tráfico
}

void CanTrafficLogger::log_can_frame(const std::string& direction,
                                     const std::string& interface,
                                     const CanFrame& frame) {
    write_line(can_frame_to_csv(direction, interface, frame));
}

std::string CanTrafficLogger::get_csv_header() const {
    return "timestamp,direction,interface,can_id,dlc,data_hex";
}

std::string CanTrafficLogger::can_frame_to_csv(const std::string& direction,
                                               const std::string& interface,
                                               const CanFrame& frame) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << direction << ",";
    csv << interface << ",";
    csv << "0x" << std::hex << std::setw(3) << std::setfill('0') << frame.id << std::dec << ",";
    csv << frame.payload.size() << ",";
    csv << payload_to_hex(frame.payload);
    
    return csv.str();
}

std::string CanTrafficLogger::payload_to_hex(const std::vector<uint8_t>& payload) const {
    std::ostringstream hex;
    
    for (size_t i = 0; i < payload.size(); ++i) {
        if (i > 0) hex << " ";
        hex << std::hex << std::setw(2) << std::setfill('0') 
            << static_cast<int>(payload[i]);
    }
    
    return hex.str();
}

} // namespace common
