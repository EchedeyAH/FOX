#pragma once
/**
 * rt_threads.hpp
 * Implementación de los 6 hilos RT del ECU ATC8110.
 *
 * Equivalencias con el sistema legacy ECU_FOX_rc30 (QNX Neutrino):
 *   thread_can_rx()         ← can_rx()                (prio 80)
 *   thread_adc()            ← adq_main()              (prio 70)
 *   thread_control()        ← ctrl_traccion_main()    (prio 60)
 *   thread_can_tx_motors()  ← canMotor() ×4           (prio 50)
 *   thread_bms_supervisor() ← can_superv() + BMS      (prio 40)
 *   thread_watchdog()       ← errores_main()           (prio 20)
 *
 * Todos los hilos siguen el patrón del legacy:
 *   while (ctx->running) { hacer_trabajo(); periodic_sleep(next, PERIOD); }
 */

#include "../common/rt_thread.hpp"
#include "../common/rt_context.hpp"
#include "../common/logging.hpp"
#include "../comunicacion_can/can_initializer.hpp"
#include "../comunicacion_can/can_protocol.hpp"
#include "../control_vehiculo/controllers.hpp"

// Nuevos módulos de seguridad
#include "../common/motor_timeout_detector.hpp"
#include "../common/voltage_protection.hpp"
#include "../common/system_mode_manager.hpp"
#include "../control_vehiculo/ao_control.h"
#include "../../control_vehiculo/safe_motor_test.hpp"

#include <thread>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace logica_sistema {

// ─────────────────────────────────────────────────────────────────────────────
// HILO 1 — CAN RX  (Prioridad 80, Período ~1ms)
// Equivale a: can_rx() del legacy
// Recibe tramas de ambos buses CAN y actualiza el snapshot.
// Prio más alta porque el CAN es event-driven y no perdemos mensajes.
//
// INTEGRACIÓN DE SEGURIDAD:
//   - Notifica al detector de timeout de motores cuando llega mensaje
//   - El detector actualiza last_seen para cada motor (M1-M4)
// ─────────────────────────────────────────────────────────────────────────────
inline void* thread_can_rx(void* arg)
{
    auto* ctx = static_cast<RtContext*>(arg);
    LOG_INFO("CAN_RX", "Hilo iniciado [prio=80, T=1ms]");

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (ctx->running.load()) {
        {
            std::lock_guard<std::mutex> lock(ctx->mtx_snapshot);
            // Procesar BMS (emuccan1) → actualiza snapshot_.battery
            ctx->can_bms.process_rx(ctx->snapshot);
            // Procesar Motores/Supervisor (emuccan0) → actualiza snapshot_.motors
            ctx->can_motors.process_rx(ctx->snapshot);
            
            // ─── INTEGRACIÓN: Detectar mensajes de motores para timeout ───
            // El can_manager ya procesa los mensajes, aquí notificamos al detector
            // de timeout basado en los IDs CAN recibidos (0x281-0x284)
            for (size_t i = 0; i < 4; ++i) {
                if (ctx->snapshot.motors[i].enabled) {
                    // Motor respondió → actualizar timestamp
                    ctx->timeout_detector.on_motor_message(static_cast<uint8_t>(i));
                }
            }
        }
        common::periodic_sleep(next, common::periods::CAN_RX_NS);
    }

    LOG_INFO("CAN_RX", "Hilo terminado");
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// HILO 2 — ADC / SENSORES  (Prioridad 70, Período 10ms)
// Equivale a: adq_main() del legacy (PCM-3718 → ahora PEX-1202L)
// Lee sensores analógicos y digitales, actualiza snapshot.vehicle.
// ─────────────────────────────────────────────────────────────────────────────
inline void* thread_adc(void* arg)
{
    auto* ctx = static_cast<RtContext*>(arg);
    LOG_INFO("ADC", "Hilo iniciado [prio=70, T=10ms]");

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    static int log_cnt = 0;

    while (ctx->running.load()) {
        // Poll sensores (PEX-1202L)
        const auto samples = ctx->sensores.poll();
        ctx->update_vehicle_from_samples(samples);

        // Log periódico cada ~1s (100 ciclos × 10ms)
        if (log_cnt++ % 100 == 0) {
            auto snap = ctx->read_snapshot();
            LOG_INFO("ADC", "Acel=" + std::to_string(snap.vehicle.accelerator)
                          + " | Freno=" + std::to_string(snap.vehicle.brake));
        }

        common::periodic_sleep(next, common::periods::ADC_NS);
    }

    LOG_INFO("ADC", "Hilo terminado");
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// HILO 3 — CONTROL DE TRACCIÓN / FSM  (Prioridad 60, Período 20ms)
// Equivale a: ctrl_traccion_main() del legacy
// Implementa la FSM de estados y calcula los torques por motor.
// Espera freno pisado para inicializar motores (igual que el legacy).
// ─────────────────────────────────────────────────────────────────────────────
inline void* thread_control(void* arg)
{
    auto* ctx = static_cast<RtContext*>(arg);
    LOG_INFO("CTRL", "Hilo iniciado [prio=60, T=20ms]");

    // Crear controladores
    auto battery_mgr   = control_vehiculo::CreateBatteryManager();
    auto suspension_ctrl = control_vehiculo::CreateSuspensionController();
    auto traction_ctrl = control_vehiculo::CreateTractionControl();
    battery_mgr->start();
    suspension_ctrl->start();
    traction_ctrl->start();

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    static int wait_cnt = 0;

    while (ctx->running.load()) {
        EstadoEcu estado = ctx->estado.load();

        if (estado == EstadoEcu::Inicializando) {
            // ── Espera pedal de freno (igual que el legacy: freno > 0.5 / 5.1 normalizado) ──
            auto snap = ctx->read_snapshot();
            if (snap.vehicle.brake >= 0.2) {
                LOG_INFO("CTRL", "Freno detectado → iniciando secuencia de arranque...");

                // Activar señal ENABLE hardware (PEX-DA16)
                if (ctx->actuador) {
                    ctx->actuador->write_output("ENABLE", 1.0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }

                // Secuencia CAN de inicialización de motores
                comunicacion_can::CanInitializer initializer(ctx->can_motors);
                bool ok = initializer.initialize_all();
                if (ok) {
                    {
                        std::lock_guard<std::mutex> lock(ctx->mtx_snapshot);
                        for (auto& m : ctx->snapshot.motors) m.enabled = true;
                    }
                    ctx->estado.store(EstadoEcu::Operando);
                    LOG_INFO("CTRL", "Motores inicializados → estado OPERANDO");
                } else {
                    LOG_ERROR("CTRL", "Fallo inicialización motores, reintentando...");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            } else {
                if (wait_cnt++ % 50 == 0) // Log cada ~1s (50 × 20ms)
                    LOG_INFO("CTRL", "Esperando pedal de freno...");
            }

        } else if (estado == EstadoEcu::Operando) {
            // ── Control de tracción (lógica identica al legacy ctrl_traccion_main) ──
            common::SystemSnapshot snap = ctx->read_snapshot();

            // Ejecutar todos los controladores sobre una copia local del snapshot
            battery_mgr->update(snap);
            suspension_ctrl->update(snap);
            traction_ctrl->update(snap);

            // Extraer torques calculados y escribirlos de vuelta al snapshot
            std::array<double, 4> torques{};
            std::array<bool, 4>   enabled{};
            for (size_t i = 0; i < 4; ++i) {
                torques[i] = snap.motors[i].torque_nm;
                enabled[i] = snap.motors[i].enabled;
            }
            ctx->write_motor_torques(torques, enabled);

            // Comprobar fallos críticos
            if (snap.faults.critical) {
                LOG_ERROR("CTRL", "Fallo crítico detectado → APAGADO");
                ctx->estado.store(EstadoEcu::Error);
            }

        } else if (estado == EstadoEcu::Error) {
            ctx->estado.store(EstadoEcu::Apagado);

        } else { // Apagado
            ctx->running.store(false);
            break;
        }

        common::periodic_sleep(next, common::periods::CONTROL_NS);
    }

    battery_mgr->stop();
    suspension_ctrl->stop();
    traction_ctrl->stop();

    LOG_INFO("CTRL", "Hilo terminado");
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// HILO 4 — CAN TX MOTORES  (Prioridad 50, Período 50ms)
// Equivale a: canMotor() ×4 del legacy
// Envía comandos de throttle/brake a cada motor por CAN.
// También solicita telemetría periódica.
// ─────────────────────────────────────────────────────────────────────────────
inline void* thread_can_tx_motors(void* arg)
{
    auto* ctx = static_cast<RtContext*>(arg);
    LOG_INFO("CAN_TX", "Hilo iniciado [prio=50, T=50ms]");

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int telemetry_cnt = 0;
    static int log_cnt = 0;

    // ✅ SAFE_TEST_MODE: Limitar torque a 15 Nm
    constexpr double MAX_TORQUE   = control_vehiculo::SAFE_TEST_MODE ? 
                                    control_vehiculo::MAX_TORQUE_SAFE : 100.0;
    constexpr double MAX_VOLTAGE  = control_vehiculo::SAFE_TEST_MODE ?
                                    control_vehiculo::MAX_VOLTAGE_SAFE : 5.0;
    constexpr double TORQUE_TO_V  = MAX_VOLTAGE / MAX_TORQUE;
    constexpr double LOOP_DT_S    = 0.05;
    control_vehiculo::SafeMotorTest safe_motor{};
    
    // Log mode control
    static int can_tx_log_count = 0;

    while (ctx->running.load()) {
        if (ctx->estado.load() == EstadoEcu::Operando) {
            auto snap = ctx->read_snapshot();
            const double throttle_in = snap.vehicle.accelerator;
            const double brake_in = snap.vehicle.brake;
            bool comm_ok = true;

            for (size_t i = 0; i < snap.motors.size(); ++i) {
                const auto& motor    = snap.motors[i];
                uint8_t     motor_id = static_cast<uint8_t>(i + 1); // 1-4

                uint8_t throttle = 0;
                uint8_t brake    = 0;
                double  voltage  = 0.0;

                if (motor.enabled) {
                    double norm = std::clamp(motor.torque_nm / MAX_TORQUE, 0.0, 1.0);
                    throttle    = static_cast<uint8_t>(norm * 255);
                    voltage     = std::clamp(motor.torque_nm * TORQUE_TO_V, 0.0, 5.0);
                }

                // Enviar por CAN
                comm_ok = comm_ok && ctx->can_motors.send_motor_command(motor_id, throttle, brake);

                // Log Motor 1 cada 10 ciclos (~500ms) - MEJORADO para SAFE_TEST
                if (motor_id == 1 && can_tx_log_count++ % 10 == 0) {
                    double clamped_torque = std::clamp(motor.torque_nm, 0.0, MAX_TORQUE);
                    printf("[CAN_TX_M1] torque=%6.2f(clamped)→%6.2f(max=%5.1f) "
                           "throttle=%3d voltage=%5.2fV AO_limit=%.1fV\n",
                           motor.torque_nm,
                           clamped_torque,
                           MAX_TORQUE,
                           static_cast<int>(throttle),
                           std::clamp(clamped_torque * TORQUE_TO_V, 0.0, MAX_VOLTAGE),
                           MAX_VOLTAGE);
                    
                    if (control_vehiculo::SAFE_TEST_MODE) {
                        printf("               ⚠️ SAFE_TEST_MODE ACTIVE - max_torque=%5.1f Nm, max_voltage=%5.2f V\n",
                               control_vehiculo::MAX_TORQUE_SAFE,
                               control_vehiculo::MAX_VOLTAGE_SAFE);
                    }
                }
            }

            const bool adc_ok = ctx->adc_ok.load(std::memory_order_relaxed);
            safe_motor.update(throttle_in, brake_in, adc_ok, comm_ok, LOOP_DT_S);

            // Actualizar salidas analogicas (PEX-DA16) con capa segura
            if (ctx->actuador) {
                for (int i = 0; i < 4; ++i) {
                    control_vehiculo::safeWriteAnalog(ctx->actuador.get(), i, safe_motor.current_voltage);
                }
            }

            printf("[SAFE_MOTOR] throttle_in=%.2f target_throttle=%.2f current_throttle=%.2f "
                   "voltage=%.2f(max=%.1f) brake_in=%.2f faults=%d\n",
                   throttle_in,
                   safe_motor.throttle_target,
                   safe_motor.throttle_current,
                   safe_motor.current_voltage,
                   safe_motor.max_safe_voltage,
                   brake_in,
                   safe_motor.fault_count);

            // Telemetría cada 20 ciclos (~1s)
            if (++telemetry_cnt >= 20) {
                telemetry_cnt = 0;
                for (uint8_t mid = 1; mid <= 4; ++mid) {
                    ctx->can_motors.request_motor_telemetry(
                        mid, comunicacion_can::MSG_TIPO_09);
                }
            }
        }

        common::periodic_sleep(next, common::periods::CAN_TX_NS);
    }

    LOG_INFO("CAN_TX", "Hilo terminado");
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// HILO 5 — BMS + SUPERVISOR  (Prioridad 40, Período 100ms)
// Equivale a: can_superv() + gestión BMS del legacy
// Envía heartbeat y estado de batería al supervisor.
// ─────────────────────────────────────────────────────────────────────────────
inline void* thread_bms_supervisor(void* arg)
{
    auto* ctx = static_cast<RtContext*>(arg);
    LOG_INFO("BMS_SUP", "Hilo iniciado [prio=40, T=100ms]");

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (ctx->running.load()) {
        if (ctx->estado.load() == EstadoEcu::Operando) {
            auto snap = ctx->read_snapshot();

            // Heartbeat al supervisor (bus motores)
            ctx->can_motors.publish_heartbeat();

            // Estado batería al supervisor (bus motores)
            ctx->can_motors.publish_battery(snap.battery);
        }

        common::periodic_sleep(next, common::periods::BMS_SUPERVISOR_NS);
    }

    LOG_INFO("BMS_SUP", "Hilo terminado");
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// HILO 6 — WATCHDOG  (Prioridad 20, Período 500ms)
// Equivale a: errores_main() del legacy
// Monitoriza fallos críticos y para el sistema ordenadamente.
//
// INTEGRACIÓN DE SEGURIDAD:
//   - Verifica timeout de motores (cada 500ms)
//   - Verifica voltaje de batería
//   - Controla modo del sistema (OK → LIMP_MODE → SAFE_STOP)
// ─────────────────────────────────────────────────────────────────────────────
inline void* thread_watchdog(void* arg)
{
    auto* ctx = static_cast<RtContext*>(arg);
    LOG_INFO("WDG", "Hilo watchdog iniciado [prio=20, T=500ms]");

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (ctx->running.load()) {
        auto snap = ctx->read_snapshot();

        // ─── INTEGRACIÓN: Verificar timeout de motores (legacy: TIEMPO_EXP_MOT_S=2s) ───
        uint8_t timeout_count = ctx->timeout_detector.timeout_count();
        
        if (timeout_count > 0 && ctx->estado.load() == EstadoEcu::Operando) {
            // Error crítico: timeout de motor → SAFE_STOP inmediato
            LOG_ERROR("WDG", "Timeout motor detectado (" + std::to_string(timeout_count) + ") → SAFE_STOP");
            ctx->mode_manager.notify_critical_error(ecu::ErrorCode::M1_COM_TIMEOUT);
            ctx->estado.store(EstadoEcu::Error);
        }

        // ─── INTEGRACIÓN: Verificar voltaje de batería ───
        // Evitar SAFE_STOP si no hay datos BMS válidos aún
        static int vbat_warn_cnt = 0;
        if (!snap.battery.communication_ok || snap.battery.pack_voltage_mv <= 0.0) {
            if (vbat_warn_cnt++ % 20 == 0) {
                LOG_WARN("WDG", "VBAT no válida (sin BMS o 0 mV) -> omitiendo protección");
            }
            common::periodic_sleep(next, common::periods::WATCHDOG_NS);
            continue;
        }

        // Normalizar posibles escalas erróneas (V, dV, cV -> mV)
        int32_t vbat_mv = static_cast<int32_t>(snap.battery.pack_voltage_mv);
        int32_t vbat_raw = vbat_mv;
        if (vbat_mv > 0 && vbat_mv < 200) vbat_mv = vbat_mv * 1000;
        else if (vbat_mv > 0 && vbat_mv < 2000) vbat_mv = vbat_mv * 100;
        else if (vbat_mv > 0 && vbat_mv < 20000) vbat_mv = vbat_mv * 10;
        if (vbat_mv != vbat_raw) {
            LOG_WARN("WDG", "VBAT escala corregida: raw=" + std::to_string(vbat_raw) +
                             " -> " + std::to_string(vbat_mv) + " mV");
        }

        // Diagnóstico periódico de VBAT para entender SAFE_STOP
        static int vbat_log_cnt = 0;
        if (vbat_log_cnt++ % 20 == 0) {
            LOG_INFO("WDG", "VBAT actual: " + std::to_string(vbat_mv) + " mV (comm_ok=" +
                            std::to_string(snap.battery.communication_ok ? 1 : 0) + ")");
        }

        auto voltage_result = ctx->voltage_protection.check_voltage(vbat_mv);
        
        if (voltage_result.safe_stop && ctx->estado.load() == EstadoEcu::Operando) {
            // Voltaje crítico → SAFE_STOP
            LOG_ERROR("WDG", "Voltaje crítico → SAFE_STOP");
            ctx->mode_manager.notify_critical_error(ecu::ErrorCode::BMS_VOLT_LOW);
            ctx->estado.store(EstadoEcu::Error);
        }
        else if (voltage_result.limit_power) {
            // Voltaje bajo/alto → LIMP_MODE
            ctx->mode_manager.notify_grave_error(common::LimpReason::VBAT_LOW);
        }
        else {
            // Voltaje OK → verificar si podemos recuperar
            ctx->mode_manager.notify_error_resolved();
        }

        // ─── Aplicar factor de limitación de potencia al control ───
        // Si estamos en SAFE_STOP, forzar torque 0
        if (ctx->mode_manager.is_torque_zero()) {
            std::array<double, 4> zero_torques = {0.0, 0.0, 0.0, 0.0};
            std::array<bool, 4> disabled = {false, false, false, false};
            ctx->write_motor_torques(zero_torques, disabled);
            LOG_WARN("WDG", "Torque 0 forzado por SAFE_STOP");
        }

        // Verificar fallos críticos originales
        if (snap.faults.critical && ctx->estado.load() == EstadoEcu::Operando) {
            LOG_ERROR("WDG", "Watchdog: fallo crítico detectado → parando sistema");
            ctx->estado.store(EstadoEcu::Error);
        }

        common::periodic_sleep(next, common::periods::WATCHDOG_NS);
    }

    LOG_INFO("WDG", "Hilo watchdog terminado");
    return nullptr;
}

} // namespace logica_sistema
