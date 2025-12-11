#pragma once

#include "../common/logging.hpp"
#include "../common/types.hpp"
#include "../comunicacion_can/can_manager.hpp"
#include "../comunicacion_can/can_initializer.hpp"
#include "../control_vehiculo/controllers.hpp"
#include "../adquisicion_datos/sensor_manager.hpp"
#include "../adquisicion_datos/pexda16.hpp"

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
    // EMUC-B202: CH1=Motors(1M), CH2=BMS(500k)
    comunicacion_can::CanManager can_motors_{"emuccan0"}; 
    comunicacion_can::CanManager can_bms_{"emuccan1"};    
    
    adquisicion_datos::SensorManager sensores_{};
    std::unique_ptr<common::IActuatorWriter> actuator_; // Actuador para ENABLE (PEX-DA16)
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
    snapshot_.battery.pack_voltage_mv = 380000; // 380V
    snapshot_.battery.pack_current_ma = 0;
    snapshot_.battery.state_of_charge = 50.0;
    
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
    
    // Inicializar motores y supervisor con secuencia de activación (PEX-DA16 + CAN)
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
        
        // --- Comunicaciones CAN ---
        
        // 1. Enviar Heartbeat y Estado Batería al Supervisor (en bus Motores)
        can_motors_.publish_heartbeat();
        can_motors_.publish_battery(snapshot_.battery);
        
        // 2. Procesar Recepción
        // BMS (en bus BMS) -> Actualiza snapshot_.battery
        can_bms_.process_rx(snapshot_);
        
        // Motores/Supervisor (en bus Motores) -> Actualiza snapshot_.motors / comandos
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
    // [DEBUG] Desactivar poll real para evitar spam de errores Pex1202L mientras se arregla el driver
    // const auto samples = sensores_.poll();
    // for (const auto &s : samples) {
    //     if (s.name == "acelerador") snapshot_.vehicle.accelerator = s.value;
    //     else if (s.name == "freno") snapshot_.vehicle.brake = s.value;
    //     else if (s.name == "volante") snapshot_.vehicle.steering = s.value;
    //     else if (s.name == "suspension_fl") snapshot_.vehicle.suspension_mm[0] = s.value;
    //     else if (s.name == "suspension_fr") snapshot_.vehicle.suspension_mm[1] = s.value;
    //     else if (s.name == "suspension_rl") snapshot_.vehicle.suspension_mm[2] = s.value;
    //     else if (s.name == "suspension_rr") snapshot_.vehicle.suspension_mm[3] = s.value;
    // }
    
    // [FORCE] Override accelerator for testing (RAMP UP)
    // Incrementa gradualmente para probar movimiento suave
    static float forced_accel = 0.0f;
    if (forced_accel < 1.0f) {
        forced_accel += 0.005f; // +0.5% cada ciclo (50ms) -> 100% en 10s
    }
    
    snapshot_.vehicle.accelerator = static_cast<double>(forced_accel);
    snapshot_.vehicle.brake = 0.0; // [FORCE] Asegurar freno suelto
    
    // Loguear para confirmar ramp-up
    static int log_cnt = 0;
    if (log_cnt++ % 20 == 0) { // Log cada ~1s
       LOG_INFO("StateMachine", "ACELERADOR (RAMP): " + std::to_string(forced_accel));
    }
}

inline void StateMachine::send_motor_commands()
{
    // [DEBUG] Counter for logging throttling
    static int log_cnt = 0;

    for (size_t i = 0; i < snapshot_.motors.size(); ++i) {
        const auto& motor = snapshot_.motors[i];
        uint8_t motor_id = static_cast<uint8_t>(i + 1); // 1-4
        
        // Analog Output Update (0-5V)
        double voltage_cmd = 0.0;

        if (motor.enabled) {
            // Convertir torque_nm a throttle (0-255) for CAN
            uint8_t throttle = torque_to_throttle(motor.torque_nm);
            uint8_t brake = 0; // Future: Map brake if needed
            
            // CAN Command
            can_motors_.send_motor_command(motor_id, throttle, brake);

            // Analog Output Command (0-5V)
            // Asumimos 100Nm = 5V (Full Scale)
            // Clamp negative values to 0
            voltage_cmd = std::max(0.0, (motor.torque_nm / 100.0) * 5.0);
            if (voltage_cmd > 5.0) voltage_cmd = 5.0;

            // [DEBUG] Loguear comando para verificar torque (solo Motor 1 para no saturar)
            if (motor_id == 1 && (log_cnt++ % 10 == 0)) { // Log cada 500ms
                 LOG_INFO("StateMachine", "TX Motor 1 -> Throttle: " + std::to_string(static_cast<int>(throttle)) + 
                          " (Torque: " + std::to_string(motor.torque_nm) + " Nm) | AO: " + std::to_string(voltage_cmd) + "V");
            }
        } else {
            // Motor deshabilitado: enviar throttle = 0
            can_motors_.send_motor_command(motor_id, 0, 0);
            voltage_cmd = 0.0;
        }

        // Write to DAQ
        if (actuator_) {
             std::string ch = "AO" + std::to_string(i); // AO0, AO1, AO2, AO3
             actuator_->write_output(ch, voltage_cmd);
        }
    }
}

inline void StateMachine::request_motor_telemetry()
{
    using namespace comunicacion_can;
    
    // Solicitar telemetría cada 20 ciclos (~1 segundo)
    if (++telemetry_counter_ >= 20) {
        telemetry_counter_ = 0;
        
        for (uint8_t motor_id = 1; motor_id <= 4; ++motor_id) {
            // Alternar entre MONITOR1 (temp) y MONITOR2 (RPM)
            // Se piden ambos tipos con frecuencia suficiente
            MotorMessageType msg_type = (telemetry_counter_ % 2 == 0) ? 
                MSG_TIPO_09 : MSG_TIPO_10;
            
            can_motors_.request_motor_telemetry(motor_id, msg_type);
        }
    }
}

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
