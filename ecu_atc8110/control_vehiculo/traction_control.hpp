#pragma once

#include <vector>
#include <cmath>
#include <array>

namespace fox {
namespace control {

struct MotorData {
    float rpm;
    float torque_cmd;
    float temperature;
};

struct VehicleData {
    float accelerator_pedal; // 0.0 to 1.0
    float brake_pedal;       // 0.0 to 1.0
    float steering_angle;    // radians
    float speed_mps;         // meters per second
    std::array<float, 3> acceleration; // x, y, z in m/s^2
    std::array<float, 3> angular_velocity; // x, y, z in rad/s
};

struct TractionConfig {
    bool enable;
    float torque_limit_nm;
    float slip_coeff_c1;
    float slip_coeff_c2;
    float slip_coeff_c3;
    float mass_kg;
    float wheel_diameter_front_m;
    float wheel_diameter_rear_m;
    float wheelbase_m;
    float track_width_front_m;
    float track_width_rear_m;
};

class TractionControl {
public:
    TractionControl();
    ~TractionControl() = default;

    void setConfig(const TractionConfig& config);
    void update(const VehicleData& vehicle_data, std::array<MotorData, 4>& motors);

private:
    TractionConfig config_;
    
    // Internal state for filters and derivatives
    std::array<float, 4> wheel_speed_history_[3]; // History for 4 wheels, 3 steps
    std::array<float, 4> slip_ratio_;
    
    float calculateSlip(float wheel_speed, float vehicle_speed);
    float calculateMaxTorque(float slip, float normal_force);
    void updateWheelSpeeds(const std::array<MotorData, 4>& motors);
};

} // namespace control
} // namespace fox
