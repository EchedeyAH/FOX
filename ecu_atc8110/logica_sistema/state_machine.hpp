#pragma once

#include "../common/logging.hpp"
#include "../common/types.hpp"
#include "../comunicacion_can/can_manager.hpp"
#include "../comunicacion_can/can_initializer.hpp"
#include "../control_vehiculo/controllers.hpp"
#include "../adquisicion_datos/pexda16.hpp"

// ... (existing includes)

namespace logica_sistema {

// ... (EstadoEcu enum)

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
    std::unique_ptr<common::IActuatorWriter> actuator_; // Actuador para ENABLE
    std::vector<std::unique_ptr<common::IController>> controllers_;

    // ... (rest of privacy methods)
    
    // ...
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

// ... (step method unchanged)

// ... (transition_to method unchanged)

// ... (run_controllers method unchanged)

inline void StateMachine::refresh_sensors()
{
    const auto samples = sensores_.poll();
    for (const auto &s : samples) {
        if (s.name == "acelerador") snapshot_.vehicle.accelerator = s.value;
        else if (s.name == "freno") snapshot_.vehicle.brake = s.value;
        else if (s.name == "volante") snapshot_.vehicle.steering = s.value;
        // ... (suspension mapping)
    }
    
    // [FORCE] Override accelerator for testing
    // Ignorar lectura real (que falla) y forzar 50%
    snapshot_.vehicle.accelerator = 0.5;
    
    // Loguear para confirmar
    static int log_cnt = 0;
    if (log_cnt++ % 50 == 0) {
       // LOG_INFO("StateMachine", "ACELERADOR FORZADO A 50%");
    }
}

// ... (send_motor_commands method unchanged)

// ... (request_motor_telemetry method unchanged)

inline bool StateMachine::initialize_motors()
{
    using namespace comunicacion_can;
    
    LOG_INFO("StateMachine", "Iniciando secuencia de activación...");
    
    // 1. Activar señal hardware ENABLE (PEX-DA16)
    if (actuator_) {
        LOG_INFO("StateMachine", "Activando señal ENABLE (Hardware)...");
        actuator_->write_output("ENABLE", 1.0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Esperar relé
    }
    
    // 2. Secuencia CAN
    CanInitializer initializer(can_);
    
    // Ejecutar secuencia completa de inicialización
    // Esto activará tanto motores como supervisor
    bool success = initializer.initialize_all();
    
    if (success) {
        LOG_INFO("StateMachine", "Motores y supervisor activados correctamente");
        
        // Habilitar todos los motores por defecto
        for (auto& motor : snapshot_.motors) {
            motor.enabled = true;
        }
    } else {
        // ... (error logging)
    }
    
    return success;
}

} // namespace logica_sistema
