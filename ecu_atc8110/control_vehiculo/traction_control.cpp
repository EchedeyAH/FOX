#include "traction_control.hpp"
#include <algorithm>
#include <iostream>

namespace fox {
namespace control {

TractionControl::TractionControl() {
    // Initialize history to zero
    for (int i = 0; i < 3; ++i) {
        wheel_speed_history_[i].fill(0.0f);
    }
    slip_ratio_.fill(0.0f);
}

void TractionControl::setConfig(const TractionConfig& config) {
    config_ = config;
}

void TractionControl::update(const VehicleData& vehicle_data, std::array<MotorData, 4>& motors) {
    if (!config_.enable) {
        return;
    }

    // Update wheel speed history
    updateWheelSpeeds(motors);

    // Basic torque distribution (placeholder for full yaw control)
    // Map accelerator to torque request
    float torque_request = vehicle_data.accelerator_pedal * config_.torque_limit_nm;

    // Calculate slip for each wheel and limit torque
    // Mapping: 0=RL, 1=FL, 2=RR, 3=FR (matches legacy code logic)
    // Legacy: FL=2, FR=4, RL=1, RR=3 -> Array indices: RL=0, FL=1, RR=2, FR=3
    
    // Estimate vehicle speed from wheels if GPS/IMU speed is low/unreliable (simplified)
    // For now use provided vehicle_speed_mps
    float v_vehicle = std::max(vehicle_data.speed_mps, 0.0f);

    for (int i = 0; i < 4; ++i) {
        // Low speed bypass: If vehicle is moving very slowly (< 1 m/s), allow torque to initiate motion
        // Slip calculation is unstable/undefined at 0 speed.
        if (v_vehicle < 1.0f) {
            motors[i].torque_cmd = torque_request;
            continue;
        }

        float wheel_radius = (i == 1 || i == 3) ? config_.wheel_diameter_front_m / 2.0f : config_.wheel_diameter_rear_m / 2.0f;
        float v_wheel = motors[i].rpm * (2.0f * 3.14159f / 60.0f) * wheel_radius;
        
        // Calculate longitudinal slip
        // slip = (v_wheel - v_vehicle) / max(v_wheel, v_vehicle)
        float slip = (v_wheel - v_vehicle) / std::max(v_wheel, v_vehicle);
        slip_ratio_[i] = slip;

        // Calculate max allowed torque based on friction coefficient (Magic Formula simplified)
        // mu = c1 * (1 - exp(-c2 * slip)) - c3 * slip
        float mu = config_.slip_coeff_c1 * (1.0f - std::exp(-config_.slip_coeff_c2 * std::abs(slip))) - config_.slip_coeff_c3 * std::abs(slip);
        
        // Normal force estimation (simplified static weight distribution)
        // Fz = m * g / 4 (assuming 50/50 distribution for now)
        float normal_force = (config_.mass_kg * 9.81f) / 4.0f;
        
        float max_traction_force = mu * normal_force;
        float max_torque = max_traction_force * wheel_radius;

        // Apply torque limit
        float final_torque = std::min(torque_request, max_torque);
        
        // Ensure we don't apply negative torque if accelerator is pressed (unless braking logic added)
        if (torque_request > 0) {
             motors[i].torque_cmd = std::max(0.0f, final_torque);
        } else {
             motors[i].torque_cmd = 0.0f;
        }
    }
}

void TractionControl::updateWheelSpeeds(const std::array<MotorData, 4>& motors) {
    // Shift history
    wheel_speed_history_[2] = wheel_speed_history_[1];
    wheel_speed_history_[1] = wheel_speed_history_[0];
    
    for (int i = 0; i < 4; ++i) {
        wheel_speed_history_[0][i] = motors[i].rpm;
    }
}

} // namespace control
} // namespace fox
