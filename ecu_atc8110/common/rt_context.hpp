#pragma once
/**
 * rt_context.hpp
 * Contexto compartido entre todos los hilos RT del ECU.
 * Reemplaza las variables globales con mutex del sistema legacy (ecu_fox.c).
 *
 * Equivalencias legacy:
 *   vehiculo     → snapshot.vehicle     (mut_vehiculo → mtx_snapshot)
 *   bateria      → snapshot.battery     (mut_bateria  → mtx_snapshot)
 *   motor1..4    → snapshot.motors[]    (mut_motor*   → mtx_snapshot)
 *   potencias    → snapshot.motors[].torque_nm
 *   errores      → snapshot.faults
 *   hilos_listos → running (std::atomic)
 */

#include "../common/types.hpp"
#include "../comunicacion_can/can_manager.hpp"
#include "../adquisicion_datos/sensor_manager.hpp"
#include "../adquisicion_datos/pexda16.hpp"
#include "../common/interfaces.hpp"

// Módulos de seguridad y protección
#include "motor_timeout_detector.hpp"
#include "voltage_protection.hpp"
#include "system_mode_manager.hpp"

#include <mutex>
#include <atomic>
#include <memory>

namespace logica_sistema {

enum class EstadoEcu {
    Inicializando,
    Operando,
    Error,
    Apagado,
};

/**
 * RtContext — Estado global compartido entre todos los hilos.
 * 
 * Acceso al snapshot: proteger siempre con mtx_snapshot.
 * Las flags atómicas (running, estado) no necesitan mutex.
 */
struct RtContext {

    // ── Estado del sistema ──────────────────────────────────────────────────
    common::SystemSnapshot snapshot{};       // Estado completo del vehículo
    mutable std::mutex mtx_snapshot;         // Protege snapshot (equivale a todos los mut_* del legacy)

    // ── Control de ciclo de vida ────────────────────────────────────────────
    std::atomic<bool>       running{false};  // true = todos los hilos activos
    std::atomic<EstadoEcu>  estado{EstadoEcu::Inicializando};
    std::atomic<bool>       adc_ok{true};

    // ── Módulos de hardware (propiedad del contexto) ────────────────────────
    comunicacion_can::CanManager can_motors{"emuccan2"};  // CAN motores 1Mbps
    comunicacion_can::CanManager can_bms{"emuccan0"};     // CAN BMS 500kbps
    adquisicion_datos::SensorManager sensores{};
    std::unique_ptr<common::IActuatorWriter> actuador;    // PEX-DA16 (opcional)

    // ── Módulos de seguridad y protección (instancias globales) ───────────
    // Referencias a las instancias globales de los módulos de seguridad
    common::MotorTimeoutDetector& timeout_detector = common::get_motor_timeout_detector();
    common::VoltageProtection& voltage_protection = common::get_voltage_protection();
    common::SystemModeManager& mode_manager = common::get_system_mode_manager();

    // ── Constructor: valores seguros por defecto ───────────────────────────
    RtContext()
    {
        // Valor inicial coherente con arquitectura ~61V para evitar disparos
        // de protección por defecto antes de recibir trama BMS válida.
        snapshot.battery.pack_voltage_mv  = 61000;
        snapshot.battery.communication_ok = false;
        snapshot.battery.pack_current_ma  = 0;
        snapshot.battery.state_of_charge  = 50.0;
        snapshot.motors = {
            common::MotorState{"M1"},
            common::MotorState{"M2"},
            common::MotorState{"M3"},
            common::MotorState{"M4"}
        };
    }

    // ── Helpers de acceso thread-safe ──────────────────────────────────────

    /** Actualiza el snapshot completo de sensores (llamado desde thread_adc) */
    void update_vehicle_from_samples(const std::vector<common::AnalogSample>& samples)
    {
        std::lock_guard<std::mutex> lock(mtx_snapshot);
        snapshot.vehicle.accelerator = 0.0;
        snapshot.vehicle.brake       = 0.0;
        bool accel_seen = false;
        for (const auto& s : samples) {
            if      (s.name == "acelerador")  { snapshot.vehicle.accelerator = s.value; accel_seen = true; }
            else if (s.name == "freno")         snapshot.vehicle.brake = std::max(snapshot.vehicle.brake, s.value);
            else if (s.name == "volante")       snapshot.vehicle.steering = s.value;
            else if (s.name == "suspension_fl") snapshot.vehicle.suspension_mm[0] = s.value;
            else if (s.name == "suspension_fr") snapshot.vehicle.suspension_mm[1] = s.value;
            else if (s.name == "suspension_rl") snapshot.vehicle.suspension_mm[2] = s.value;
            else if (s.name == "suspension_rr") snapshot.vehicle.suspension_mm[3] = s.value;
        }
        adc_ok.store(!samples.empty() && accel_seen, std::memory_order_relaxed);
    }

    /** Copia el snapshot para uso local dentro de un hilo (lectura segura) */
    common::SystemSnapshot read_snapshot() const
    {
        std::lock_guard<std::mutex> lock(mtx_snapshot);
        return snapshot;
    }

    /** Escribe torques calculados de vuelta al snapshot (desde thread_control) */
    void write_motor_torques(const std::array<double, 4>& torques, const std::array<bool, 4>& enabled)
    {
        std::lock_guard<std::mutex> lock(mtx_snapshot);
        for (size_t i = 0; i < 4; ++i) {
            snapshot.motors[i].torque_nm = torques[i];
            snapshot.motors[i].enabled   = enabled[i];
        }
    }
};

} // namespace logica_sistema
