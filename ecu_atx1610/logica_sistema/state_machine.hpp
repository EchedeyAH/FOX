#pragma once

#include "../common/logging.hpp"
#include "../common/types.hpp"
#include "../comunicacion_can/can_manager.hpp"
#include "../control_vehiculo/controllers.hpp"
#include "../adquisicion_datos/sensor_manager.hpp"

#include <memory>
#include <vector>
#include <algorithm>
#include <thread>
#include <chrono>

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
    comunicacion_can::CanManager can_{"emuccan0"};  // Hardware real EMUC-B2S3
    adquisicion_datos::SensorManager sensores_{};
    std::vector<std::unique_ptr<common::IController>> controllers_;

    void transition_to(EstadoEcu nuevo_estado);
    void run_controllers();
    void refresh_sensors();
    void send_motor_commands();
    void request_motor_telemetry();
    bool initialize_motors();
    
    // Helper para convertir torque a throttle
    uint8_t torque_to_throttle(double torque_nm) const {
        constexpr double MAX_TORQUE = 100.0; // Nm
        double normalized = std::clamp(torque_nm / MAX_TORQUE, 0.0, 1.0);
        return static_cast<uint8_t>(normalized * 255);
    }
    
    int telemetry_counter_{0};
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
    
    // Inicializar motores
    initialize_motors();
    
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
        send_motor_commands();        // NUEVO: Enviar comandos a motores
        request_motor_telemetry();    // NUEVO: Solicitar telemetría
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

inline void StateMachine::send_motor_commands()
{
    for (size_t i = 0; i < snapshot_.motors.size(); ++i) {
        const auto& motor = snapshot_.motors[i];
        uint8_t motor_id = static_cast<uint8_t>(i + 1); // 1-4
        
        if (motor.enabled) {
            // Convertir torque_nm a throttle (0-255)
            uint8_t throttle = torque_to_throttle(motor.torque_nm);
            uint8_t brake = 0; // Por ahora sin freno regenerativo
            
            can_.send_motor_command(motor_id, throttle, brake);
        } else {
            // Motor deshabilitado: enviar throttle = 0
            can_.send_motor_command(motor_id, 0, 0);
        }
    }
}

inline void StateMachine::request_motor_telemetry()
{
    using namespace comunicacion_can;
    
    // Solicitar telemetría cada 100 ciclos (~1 segundo a 10ms/ciclo)
    if (++telemetry_counter_ >= 100) {
        telemetry_counter_ = 0;
        
        for (uint8_t motor_id = 1; motor_id <= 4; ++motor_id) {
            // Alternar entre MONITOR1 (temp) y MONITOR2 (RPM)
            MotorMessageType msg_type = (motor_id % 2 == 0) ? 
                MSG_TIPO_09 : MSG_TIPO_10;
            
            can_.request_motor_telemetry(motor_id, msg_type);
        }
    }
}

inline bool StateMachine::initialize_motors()
{
    using namespace comunicacion_can;
    
    LOG_INFO("StateMachine", "Inicializando motores...");
    
    // Solicitar información básica de cada motor
    for (uint8_t motor_id = 1; motor_id <= 4; ++motor_id) {
        // Leer modelo del controlador (MSG_TIPO_01)
        if (!can_.request_motor_telemetry(motor_id, MSG_TIPO_01)) {
            LOG_WARN("StateMachine", "No se pudo contactar motor " + 
                    std::to_string(motor_id));
        }
        
        // Pequeña pausa entre solicitudes
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    LOG_INFO("StateMachine", "Inicialización de motores completada");
    return true;
}

} // namespace logica_sistema
