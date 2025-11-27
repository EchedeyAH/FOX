#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace common {

using Clock = std::chrono::steady_clock;
using Timestamp = std::chrono::time_point<Clock>;

struct CanFrame {
    uint32_t id{0};
    std::vector<uint8_t> payload{};
    bool extended{false};
};

struct AnalogSample {
    std::string name;
    double value{0.0};
    Timestamp timestamp{Clock::now()};
};

struct BatteryState {
    // Comunicación
    bool communication_ok{true};
    bool bms_error{false};
    
    // Datos del pack completo
    double pack_voltage_mv{0.0};
    double pack_current_ma{0.0};
    double state_of_charge{0.0};  // Porcentaje 0-100
    
    // Alarmas
    uint8_t alarm_level{0};  // 0=NO_ALARMA, 1=WARNING, 2=ALARMA, 3=CRITICA
    uint8_t alarm_type{0};   // Tipo específico de alarma
    
    // Datos por celda (24 celdas)
    std::array<uint16_t, 24> cell_voltages_mv{{0}};
    std::array<uint8_t, 24> cell_temperatures_c{{0}};
    
    // Estadísticas
    uint8_t num_cells_detected{0};
    uint8_t temp_avg_c{0};
    uint8_t temp_max_c{0};
    uint8_t cell_temp_max_id{0};
    
    uint16_t voltage_avg_mv{0};
    uint16_t voltage_max_mv{0};
    uint16_t voltage_min_mv{0};
    uint8_t cell_v_max_id{0};
    uint8_t cell_v_min_id{0};
    
    uint16_t timestamp{0};
    Timestamp last_update{Clock::now()};
};

struct MotorState {
    std::string label;
    double rpm{0.0};
    double torque_nm{0.0};
    double inverter_temp_c{0.0};
    double motor_temp_c{0.0};
    bool enabled{false};
    Timestamp timestamp{Clock::now()};
};

struct VehicleState {
    double accelerator{0.0};
    double brake{0.0};
    double steering{0.0};
    std::array<double, 4> suspension_mm{ {0.0, 0.0, 0.0, 0.0} };
    bool reverse{false};
    Timestamp timestamp{Clock::now()};
};

struct ImuData {
    std::array<double, 3> accel_g{ {0.0, 0.0, 0.0} };
    std::array<double, 3> gyro_rad_s{ {0.0, 0.0, 0.0} };
    std::array<double, 3> euler_rad{ {0.0, 0.0, 0.0} };
    Timestamp timestamp{Clock::now()};
};

struct PowerBudget {
    double demanded_w{0.0};
    double available_w{0.0};
    double battery_w{0.0};
    double motors_w{0.0};
};

struct FaultFlags {
    bool warning{false};
    bool error{false};
    bool critical{false};
    std::string description;
};

struct SystemSnapshot {
    BatteryState battery;
    std::array<MotorState, 4> motors;
    VehicleState vehicle;
    ImuData imu;
    PowerBudget power;
    FaultFlags faults;
};

} // namespace common
