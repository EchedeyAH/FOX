#include "bms_data_logger.hpp"

namespace common {

BmsDataLogger::BmsDataLogger(const fs::path& session_directory) 
    : CsvLoggerBase({session_directory, "bms", 1000, 100, 60}) {
}

void BmsDataLogger::log_battery_state(const BatteryState& state) {
    write_line(battery_state_to_csv(state));
}

std::string BmsDataLogger::get_csv_header() const {
    std::ostringstream header;
    header << "timestamp,communication_ok,bms_error,pack_voltage_mv,pack_current_ma,soc";
    
    // Voltajes de celdas (24 celdas)
    for (int i = 0; i < 24; ++i) {
        header << ",cell_" << i << "_mv";
    }
    
    // Temperaturas de celdas (24 celdas)
    for (int i = 0; i < 24; ++i) {
        header << ",cell_" << i << "_temp_c";
    }
    
    // Estadísticas
    header << ",num_cells,temp_avg_c,temp_max_c,cell_temp_max_id";
    header << ",voltage_avg_mv,voltage_max_mv,voltage_min_mv";
    header << ",cell_v_max_id,cell_v_min_id";
    header << ",alarm_level,alarm_type";
    
    return header.str();
}

std::string BmsDataLogger::battery_state_to_csv(const BatteryState& state) const {
    std::ostringstream csv;
    
    csv << get_timestamp() << ",";
    csv << (state.communication_ok ? 1 : 0) << ",";
    csv << (state.bms_error ? 1 : 0) << ",";
    csv << state.pack_voltage_mv << ",";
    csv << state.pack_current_ma << ",";
    csv << state.state_of_charge;
    
    // Voltajes de celdas
    for (const auto& voltage : state.cell_voltages_mv) {
        csv << "," << voltage;
    }
    
    // Temperaturas de celdas
    for (const auto& temp : state.cell_temperatures_c) {
        csv << "," << static_cast<int>(temp);
    }
    
    // Estadísticas
    csv << "," << static_cast<int>(state.num_cells_detected);
    csv << "," << static_cast<int>(state.temp_avg_c);
    csv << "," << static_cast<int>(state.temp_max_c);
    csv << "," << static_cast<int>(state.cell_temp_max_id);
    csv << "," << state.voltage_avg_mv;
    csv << "," << state.voltage_max_mv;
    csv << "," << state.voltage_min_mv;
    csv << "," << static_cast<int>(state.cell_v_max_id);
    csv << "," << static_cast<int>(state.cell_v_min_id);
    csv << "," << static_cast<int>(state.alarm_level);
    csv << "," << static_cast<int>(state.alarm_type);
    
    return csv.str();
}

} // namespace common
