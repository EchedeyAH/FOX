#include "../control_vehiculo/traction_control.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace fox::control;

void testZeroTorqueWhenStopped() {
    TractionControl tc;
    TractionConfig config;
    config.enable = true;
    config.torque_limit_nm = 100.0f;
    config.slip_coeff_c1 = 1.0f;
    config.slip_coeff_c2 = 10.0f;
    config.slip_coeff_c3 = 0.0f;
    config.mass_kg = 1000.0f;
    config.wheel_diameter_front_m = 0.5f;
    config.wheel_diameter_rear_m = 0.5f;
    tc.setConfig(config);

    VehicleData vehicle_data;
    vehicle_data.accelerator_pedal = 1.0f;
    vehicle_data.speed_mps = 0.0f;
    
    std::array<MotorData, 4> motors;
    for(auto& m : motors) { m.rpm = 0; m.torque_cmd = 0; }
    
    tc.update(vehicle_data, motors);
    
    if (std::abs(motors[0].torque_cmd - 100.0f) < 0.1f) {
        std::cout << "[PASS] ZeroTorqueWhenStopped: Torque=" << motors[0].torque_cmd << " (Expected 100)" << std::endl;
    } else {
        std::cout << "[FAIL] ZeroTorqueWhenStopped: Torque=" << motors[0].torque_cmd << " (Expected 100)" << std::endl;
    }
}

void testTorqueLimitingOnSlip() {
    TractionControl tc;
    TractionConfig config;
    config.enable = true;
    config.torque_limit_nm = 100.0f;
    config.slip_coeff_c1 = 0.1f; // Low friction
    config.slip_coeff_c2 = 10.0f;
    config.slip_coeff_c3 = 0.0f;
    config.mass_kg = 1000.0f;
    config.wheel_diameter_front_m = 0.5f;
    config.wheel_diameter_rear_m = 0.5f;
    tc.setConfig(config);

    VehicleData vehicle_data;
    vehicle_data.accelerator_pedal = 1.0f;
    vehicle_data.speed_mps = 10.0f;
    
    std::array<MotorData, 4> motors;
    // Slip condition: Wheel 20m/s, Vehicle 10m/s
    float wheel_rpm = (20.0f / (0.5f/2.0f)) * (60.0f / (2.0f * 3.14159f));
    for(auto& m : motors) { m.rpm = wheel_rpm; m.torque_cmd = 0; }
    
    tc.update(vehicle_data, motors);
    
    if (motors[0].torque_cmd < 100.0f) {
        std::cout << "[PASS] TorqueLimitingOnSlip: Torque=" << motors[0].torque_cmd << " < 100" << std::endl;
    } else {
        std::cout << "[FAIL] TorqueLimitingOnSlip: Torque=" << motors[0].torque_cmd << std::endl;
    }
}

int main() {
    testZeroTorqueWhenStopped();
    testTorqueLimitingOnSlip();
    return 0;
}
