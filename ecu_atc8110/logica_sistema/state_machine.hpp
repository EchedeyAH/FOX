#pragma once

#include "../common/logging.hpp"
#include "../common/types.hpp"
#include "../comunicacion_can/can_manager.hpp"
#include "../comunicacion_can/can_initializer.hpp"
#include "../control_vehiculo/controllers.hpp"
#include "../adquisicion_datos/sensor_manager.hpp"
#include "../adquisicion_datos/pexda16.hpp"  // [NEW] Include

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
    
    // Arquitectura Dual-CAN
    comunicacion_can::CanManager can_motors_{"emuccan0"}; // CAN1 (Motors/Supervisor) @ 1M
    comunicacion_can::CanManager can_bms_{"emuccan1"};    // CAN2 (BMS) @ 500k
    
    adquisicion_datos::SensorManager sensores_{};
    std::unique_ptr<common::IActuatorWriter> actuator_; // Actuador para ENABLE
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
    // Valores por defecto seguros para batería
    snapshot_.battery.pack_voltage_mv = 380000; // mV
    snapshot_.battery.pack_current_ma = 0;      // mA
    snapshot_.battery.state_of_charge = 50.0;   // %
    
    snapshot_.motors = {common::MotorState{"M1"}, common::MotorState{"M2"},
                        common::MotorState{"M3"}, common::MotorState{"M4"}};
}

inline void StateMachine::start()
{
    // Iniciar ambos buses CAN
    if (can_motors_.start()) {
        LOG_INFO("StateMachine", "CAN Motores (emuccan0) iniciado");
    } else {
        LOG_ERROR("StateMachine", "Fallo al iniciar CAN Motores");
    }
    
    if (can_bms_.start()) {
        LOG_INFO("StateMachine", "CAN BMS (emuccan1) iniciado");
    } else {
        LOG_ERROR("StateMachine", "Fallo al iniciar CAN BMS");
    }

    sensores_.start();
    
    // Inicializar PEX-DA16
    actuator_ = adquisicion_datos::CreatePexDa16();
    if (actuator_->start()) {
        LOG_INFO("StateMachine", "PEX-DA16 inicializada correctamente");
    } else {
        LOG_ERROR("StateMachine", "Fallo al inicializar PEX-DA16");
    }

    controllers_.push_back(control_vehiculo::CreateBatteryManager());
    controllers_.push_back(control_vehiculo::CreateSuspensionController());
    controllers_.push_back(control_vehiculo::CreateTractionControl());
    for (auto &ctrl : controllers_) {
        ctrl->start();
    }
    
    // Inicializar motores y supervisor con secuencia de activación
    if (!initialize_motors()) {
        LOG_WARN("StateMachine", "Algunos motores no respondieron durante la inicialización");
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
        send_motor_commands();
        request_motor_telemetry();
        
        // Heartbeat y batería hacia Supervisor (CAN1)
        can_motors_.publish_heartbeat();
        can_motors_.publish_battery(snapshot_.battery);
        
        // Recepción:
        // BMS (CAN2) -> Actualiza snapshot_.battery
        can_bms_.process_rx(snapshot_);
        
        // Motores/Supervisor (CAN1) -> Actualiza snapshot_.motors / comandos
        can_motors_.process_rx(snapshot_);
        
        if (snapshot_.faults.critical) {
            transition_to(EstadoEcu::Error);
        }
        break;
    case EstadoEcu::Error:
        // LOG_ERROR_ONCE("FSM", "Estado de error detectado: " + snapshot_.faults.description);
        transition_to(EstadoEcu::Apagado);
        break;
    case EstadoEcu::Apagado:
        // noop
        break;
    }
}
// ... (transition_to, run_controllers, refresh_sensors unchanged)

inline void StateMachine::send_motor_commands()
{
    for (size_t i = 0; i < snapshot_.motors.size(); ++i) {
        const auto& motor = snapshot_.motors[i];
        uint8_t motor_id = static_cast<uint8_t>(i + 1); // 1-4
        
        if (motor.enabled) {
            // Convertir torque_nm a throttle (0-255)
            uint8_t throttle = torque_to_throttle(motor.torque_nm);
            uint8_t brake = 0; // Por ahora sin freno regenerativo
            
            // [DEBUG] Loguear comando para verificar torque
            if (motor_id == 1 && (static_cast<int>(throttle) > 0)) {
                // LOG_INFO("StateMachine", "TX Motor 1 -> Throttle: " + std::to_string(static_cast<int>(throttle)));
            }
            
            can_motors_.send_motor_command(motor_id, throttle, brake);
        } else {
            // Motor deshabilitado: enviar throttle = 0
            can_motors_.send_motor_command(motor_id, 0, 0);
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
            
            can_motors_.request_motor_telemetry(motor_id, msg_type);
        }
    }
}

inline bool StateMachine::initialize_motors()
{
    using namespace comunicacion_can;
    
    LOG_INFO("StateMachine", "Iniciando secuencia de activación...");
    
    // 1. [NEW] Activar señal hardware ENABLE (PEX-DA16)
    if (actuator_) {
        LOG_INFO("StateMachine", "Activando señal ENABLE (Hardware)...");
        actuator_->write_output("ENABLE", 1.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Esperar relé
    }
    
    // 2. Secuencia CAN (usando can_motors_)
    CanInitializer initializer(can_motors_);
    
    // Ejecutar secuencia completa de inicialización
    bool success = initializer.initialize_all();
    
    if (success) {
        LOG_INFO("StateMachine", "Motores y supervisor activados correctamente");
        for (auto& motor : snapshot_.motors) {
            motor.enabled = true;
        }
    } else {
        LOG_ERROR("StateMachine", "Error en la activación de motores/supervisor");
    }
    
    return success;
}

} // namespace logica_sistema
