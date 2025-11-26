#pragma once

#include "../common/logging.hpp"
#include "../common/types.hpp"
#include "../comunicacion_can/can_manager.hpp"
#include "../control_vehiculo/controllers.hpp"
#include "../adquisicion_datos/sensor_manager.hpp"

#include <memory>
#include <vector>

namespace logica_sistema {

enum class EstadoEcu {
    Inicializando,
    Operando,
    Error,
    Apagado,
};

class StateMachine {
public:
    StateMachine();
    void start();
    void step();

private:
    EstadoEcu estado_{EstadoEcu::Inicializando};
    common::SystemSnapshot snapshot_{};
    comunicacion_can::CanManager can_{"vcan0"};
    adquisicion_datos::SensorManager sensores_{};
    std::vector<std::unique_ptr<common::IController>> controllers_;

    void transition_to(EstadoEcu nuevo_estado);
    void run_controllers();
    void refresh_sensors();
};

inline StateMachine::StateMachine()
{
    snapshot_.battery.pack_voltage_mv = 380000; // mV
    snapshot_.battery.pack_current_ma = 10000;  // mA
    snapshot_.battery.state_of_charge = 80.0;
    snapshot_.motors = {common::MotorState{"M1"}, common::MotorState{"M2"},
                        common::MotorState{"M3"}, common::MotorState{"M4"}};
}

inline void StateMachine::start()
{
    can_.start();
    sensores_.start();
    controllers_.push_back(control_vehiculo::CreateBatteryManager());
    controllers_.push_back(control_vehiculo::CreateSuspensionController());
    controllers_.push_back(control_vehiculo::CreateTractionControl());
    for (auto &ctrl : controllers_) {
        ctrl->start();
    }
    transition_to(EstadoEcu::Operando);
}

inline void StateMachine::step()
{
    switch (estado_) {
    case EstadoEcu::Inicializando:
        transition_to(EstadoEcu::Operando);
        break;
    case EstadoEcu::Operando:
        refresh_sensors();
        run_controllers();
        can_.publish_heartbeat();
        can_.publish_battery(snapshot_.battery);
        can_.process_rx(snapshot_);
        if (snapshot_.faults.critical) {
            transition_to(EstadoEcu::Error);
        }
        break;
    case EstadoEcu::Error:
        LOG_ERROR("FSM", "Estado de error detectado: " + snapshot_.faults.description);
        transition_to(EstadoEcu::Apagado);
        break;
    case EstadoEcu::Apagado:
        // noop
        break;
    }
}

inline void StateMachine::transition_to(EstadoEcu nuevo_estado)
{
    estado_ = nuevo_estado;
    LOG_INFO("FSM", "Transición a estado " + std::to_string(static_cast<int>(estado_)));
}

inline void StateMachine::run_controllers()
{
    for (auto &ctrl : controllers_) {
        ctrl->update(snapshot_);
    }
}

inline void StateMachine::refresh_sensors()
{
    const auto samples = sensores_.poll();
    for (const auto &s : samples) {
        if (s.name == "acelerador") snapshot_.vehicle.accelerator = s.value;
        else if (s.name == "freno") snapshot_.vehicle.brake = s.value;
        else if (s.name == "volante") snapshot_.vehicle.steering = s.value;
        else if (s.name == "suspension_fl") snapshot_.vehicle.suspension_mm[0] = s.value;
        else if (s.name == "suspension_fr") snapshot_.vehicle.suspension_mm[1] = s.value;
        else if (s.name == "suspension_rl") snapshot_.vehicle.suspension_mm[2] = s.value;
        else if (s.name == "suspension_rr") snapshot_.vehicle.suspension_mm[3] = s.value;
    }
}

} // namespace logica_sistema
