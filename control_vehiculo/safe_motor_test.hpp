#pragma once

#include "../common/interfaces.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace control_vehiculo {

enum class MotorState {
    IDLE,
    ARMED,
    RUNNING
};

struct SafeMotorTest {
    MotorState state{MotorState::IDLE};
    double current_voltage{0.0};
    double target_voltage{0.0};
    double stable_time_s{0.0};

    double max_safe_voltage{2.0};
    double ramp_rate_v_per_s{0.5};
    double deadzone{0.05};
    double stable_required_s{2.0};
    double brake_pressed_threshold{0.2};

    bool update(double throttle, double brake, bool adc_ok, bool comm_ok, double dt)
    {
        if (!adc_ok || !comm_ok || throttle < 0.0 || throttle > 1.0) {
            state = MotorState::IDLE;
            current_voltage = 0.0;
            target_voltage = 0.0;
            stable_time_s = 0.0;
            return false;
        }

        if (std::fabs(throttle) < deadzone) {
            throttle = 0.0;
        }

        const bool safe_condition = (brake >= brake_pressed_threshold) || (throttle == 0.0);

        switch (state) {
        case MotorState::IDLE:
            if (safe_condition) {
                state = MotorState::ARMED;
                stable_time_s = 0.0;
            }
            break;
        case MotorState::ARMED:
            if (!safe_condition) {
                state = MotorState::IDLE;
                stable_time_s = 0.0;
            } else {
                stable_time_s += dt;
                if (stable_time_s >= stable_required_s) {
                    state = MotorState::RUNNING;
                }
            }
            break;
        case MotorState::RUNNING:
            break;
        }

        target_voltage = (state == MotorState::RUNNING) ? (throttle * max_safe_voltage) : 0.0;
        target_voltage = std::clamp(target_voltage, 0.0, max_safe_voltage);

        double delta = target_voltage - current_voltage;
        const double max_step = ramp_rate_v_per_s * dt;
        if (std::fabs(delta) > max_step) {
            delta = (delta > 0.0 ? 1.0 : -1.0) * max_step;
        }

        current_voltage += delta;
        current_voltage = std::clamp(current_voltage, 0.0, max_safe_voltage);
        return true;
    }
};

inline void safeWriteAnalog(common::IActuatorWriter* actuator, int channel, double voltage)
{
    if (!actuator) {
        return;
    }
    const std::string ch = "AO" + std::to_string(channel);
    actuator->write_output(ch, voltage);
}