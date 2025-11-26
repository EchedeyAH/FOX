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
    bool communication_ok{true};
    double pack_voltage_mv{0.0};
    double pack_current_ma{0.0};
    double state_of_charge{0.0};
    uint8_t alarm_level{0};
    Timestamp timestamp{Clock::now()};
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
