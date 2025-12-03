#include "system_data_logger.hpp"

namespace common {

SystemDataLogger::SystemDataLogger(const std::filesystem::path& session_directory)
    : CsvLoggerBase({session_directory, "system", 500, 100, 60}) {
}

void SystemDataLogger::log_system_snapshot(const PowerBudget& power, const FaultFlags& faults) {
    write_line(system_to_csv(power, faults));
}

std::string SystemDataLogger::get_csv_header() const {
    return "timestamp,demanded_w,available_w,battery_w,motors_w,warning,error,critical,fault_description";
}

std::string SystemDataLogger::system_to_csv(const PowerBudget& power, const FaultFlags& faults) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << power.demanded_w << ",";
    csv << power.available_w << ",";
    csv << power.battery_w << ",";
    csv << power.motors_w << ",";
    csv << (faults.warning ? 1 : 0) << ",";
    csv << (faults.error ? 1 : 0) << ",";
    csv << (faults.critical ? 1 : 0) << ",";
    
    // Escapar comillas en la descripción
    std::string escaped_desc = faults.description;
    size_t pos = 0;
    while ((pos = escaped_desc.find('"', pos)) != std::string::npos) {
        escaped_desc.replace(pos, 1, "\"\"");
        pos += 2;
    }
    csv << "\"" << escaped_desc << "\"";
    
    return csv.str();
}

} // namespace common
