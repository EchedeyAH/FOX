#include "controllers.hpp"
#include "traction_control.hpp"
#include "../common/logging.hpp"
#include "../common/types.hpp"

#include <algorithm>
#include <cmath>

namespace control_vehiculo {

class TractionControlWrapper : public common::IController {
public:
    TractionControlWrapper() {
        // Configure traction control with values from config/ecu_config.yaml
        fox::control::TractionConfig tc_config;
        tc_config.enable = true;
        tc_config.torque_limit_nm = 120.0f;
        tc_config.slip_coeff_c1 = 1.1973f;
        tc_config.slip_coeff_c2 = 25.168f;
        tc_config.slip_coeff_c3 = 0.5373f;
        tc_config.mass_kg = 1000.0f;
        tc_config.wheel_diameter_front_m = 0.5f;
        tc_config.wheel_diameter_rear_m = 0.5f;
        tc_config.wheelbase_m = 2.5f;
        tc_config.track_width_front_m = 1.5f;
        tc_config.track_width_rear_m = 1.5f;
        
        traction_control_.setConfig(tc_config);
    }

    bool start() override
    {
        LOG_INFO("TractionControl", "Inicializando control de tracción");
        return true;
    }

    void stop() override
    {
        LOG_INFO("TractionControl", "Control de tracción detenido");
    }

    void update(common::SystemSnapshot &snapshot) override
    {
        // Convert SystemSnapshot to VehicleData
        fox::control::VehicleData vehicle_data;
        vehicle_data.brake_pedal = static_cast<float>(snapshot.vehicle.brake);
        vehicle_data.accelerator_pedal = static_cast<float>(snapshot.vehicle.accelerator); // [FIX] Added assignment
        vehicle_data.steering_angle = static_cast<float>(snapshot.vehicle.steering);

        // Estimate vehicle speed from motor RPMs (average of all wheels)
        // This is a simplified approach; ideally use GPS/IMU data
        float avg_rpm = 0.0f;
        for (const auto& motor : snapshot.motors) {
            avg_rpm += static_cast<float>(motor.rpm);
        }
        avg_rpm /= 4.0f;
        
        // Convert RPM to m/s (assuming 0.5m wheel diameter)
        // v = (RPM * 2π * r) / 60
        const float wheel_radius = 0.25f; // 0.5m diameter / 2
        vehicle_data.speed_mps = (avg_rpm * 2.0f * 3.14159f * wheel_radius) / 60.0f;
        
        // IMU data (if available, otherwise zeros)
        vehicle_data.acceleration = {
            static_cast<float>(snapshot.imu.accel_g[0]),
            static_cast<float>(snapshot.imu.accel_g[1]),
            static_cast<float>(snapshot.imu.accel_g[2])
        };
        vehicle_data.angular_velocity = {
            static_cast<float>(snapshot.imu.gyro_rad_s[0]),
            static_cast<float>(snapshot.imu.gyro_rad_s[1]),
            static_cast<float>(snapshot.imu.gyro_rad_s[2])
        };
        
        // Convert motors to MotorData array
        std::array<fox::control::MotorData, 4> motors;
        for (size_t i = 0; i < 4; ++i) {
            motors[i].rpm = static_cast<float>(snapshot.motors[i].rpm);
            motors[i].torque_cmd = static_cast<float>(snapshot.motors[i].torque_nm);
            motors[i].temperature = static_cast<float>(snapshot.motors[i].motor_temp_c);
        }
        
        // Run traction control algorithm
        traction_control_.update(vehicle_data, motors);
        
        // Copy torque commands back to snapshot
        for (size_t i = 0; i < 4; ++i) {
            snapshot.motors[i].torque_nm = static_cast<double>(motors[i].torque_cmd);
        }
        
        // Log debug info periodically
        static int log_counter = 0;
        if (++log_counter >= 10) { // Every ~0.5 seconds at 50ms cycle
            log_counter = 0;
            LOG_INFO("TractionControl", 
                "Accel: " + std::to_string(vehicle_data.accelerator_pedal) + 
                " | Speed: " + std::to_string(vehicle_data.speed_mps) + " m/s" +
                " | Torque[0]: " + std::to_string(snapshot.motors[0].torque_nm) + " Nm");
        }
    }

private:
    fox::control::TractionControl traction_control_;
};

std::unique_ptr<common::IController> CreateTractionControl()
{
    return std::make_unique<TractionControlWrapper>();
}

} // namespace control_vehiculo
