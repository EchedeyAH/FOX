/**
 * main.cpp — ECU ATC8110 con arquitectura RT Multi-Hilo
 *
 * Refactorizado desde bucle single-thread a 6 hilos POSIX SCHED_FIFO.
 * Equivalente al sistema legacy ECU_FOX_rc30 sobre QNX Neutrino,
 * adaptado a Ubuntu 18.04 lowlatency (PREEMPT).
 *
 * Jerarquía de hilos (mayor prioridad = primero en ejecutar):
 *   [80] thread_can_rx         — Recepción CAN (motors + BMS)
 *   [70] thread_adc            — Lectura sensores PEX-1202L
 *   [60] thread_control        — FSM + control de tracción
 *   [50] thread_can_tx_motors  — Envío comandos CAN + salidas analógicas
 *   [40] thread_bms_supervisor — Heartbeat + estado batería al supervisor
 *   [20] thread_watchdog       — Monitor de fallos críticos
 *
 * Requisitos de ejecución:
 *   sudo ./ecu_atc8110
 *   — o bien —
 *   sudo setcap cap_sys_nice+ep ./ecu_atc8110
 */

#include "rt_threads.hpp"
#include "../common/rt_thread.hpp"
#include "../common/rt_context.hpp"
#include "../common/logging.hpp"
#include "../common/logger.hpp"
#include "../adquisicion_datos/pexda16.hpp"
#include "../common/error_publisher.hpp"

#include <pthread.h>
#include <csignal>
#include <cstring>
#include <iostream>

// ── Referencia global al contexto para el manejador de señal ──────────────
static logica_sistema::RtContext* g_ctx = nullptr;

static void signal_handler(int sig)
{
    if (g_ctx) {
        LOG_INFO("MAIN", "Señal " + std::to_string(sig) + " recibida → parando sistema...");
        g_ctx->running.store(false);
    }
    // Detener publisher de errores
    ecu::g_error_publisher.stop();
}

int main()
{
    if (!common::Logger::Instance().Init()) {
        std::cerr << "[WARN] [MAIN] No se pudo inicializar logger (datos_pruebas)" << std::endl;
    }

    std::cout << "╔══════════════════════════════════╗\n"
              << "║   ECU ATC8110 — RT Multi-Hilo   ║\n"
              << "║   SCHED_FIFO / PREEMPT kernel   ║\n"
              << "╚══════════════════════════════════╝\n" << std::endl;

    // ── 0. Inicializar sistema de errores (UNIX socket /run/ecu/errors.sock) ──
    LOG_INFO("MAIN", "Inicializando sistema de errores...");
    ecu::ErrorPublisherConfig pub_config;
    pub_config.socket_path = "/run/ecu/errors.sock";
    pub_config.send_snap_on_connect = true;
    pub_config.snap_interval_ms = 1000;
    
    if (ecu::g_error_publisher.init(pub_config)) {
        ecu::g_error_publisher.start();
        LOG_INFO("MAIN", "Error Publisher: /run/ecu/errors.sock");
    } else {
        LOG_WARN("MAIN", "Error Publisher no disponible");
    }

    // ── 1. Bloquear páginas de memoria en RAM (evita page faults en RT) ───
    common::lock_memory();

    // ── 2. Inicializar contexto compartido ────────────────────────────────
    logica_sistema::RtContext ctx;
    g_ctx = &ctx;

    // ── 3. Registrar manejadores de señal para parada ordenada ───────────
    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);

    // ── 4. Iniciar hardware ───────────────────────────────────────────────
    LOG_INFO("MAIN", "Iniciando buses CAN...");
    if (ctx.can_motors.start())
        LOG_INFO("MAIN", "CAN Motores (emuccan2 @ 1Mbps) OK");
    else
        LOG_ERROR("MAIN", "CAN Motores: fallo al iniciar");

    if (ctx.can_bms.start())
        LOG_INFO("MAIN", "CAN BMS (emuccan0 @ 500kbps) OK");
    else
        LOG_ERROR("MAIN", "CAN BMS: fallo al iniciar");

    LOG_INFO("MAIN", "Iniciando sensores (PEX-1202L)...");
    ctx.sensores.start();

    LOG_INFO("MAIN", "Iniciando actuador (PEX-DA16)...");
    ctx.actuador = adquisicion_datos::CreatePexDa16();
    if (ctx.actuador && ctx.actuador->start())
        LOG_INFO("MAIN", "PEX-DA16 OK (salidas analógicas activas)");
    else {
        LOG_WARN("MAIN", "PEX-DA16 no disponible — control solo por CAN");
        ctx.actuador = nullptr;
    }

    // ── 5. Activar flag de ejecución ──────────────────────────────────────
    ctx.running.store(true);

    // ── 6. Crear hilos RT (en orden de prioridad descendente) ─────────────
    LOG_INFO("MAIN", "Creando hilos RT con política SCHED_FIFO...");

    pthread_t thr_can_rx, thr_adc, thr_control,
              thr_can_tx, thr_bms_sup, thr_watchdog;

    try {
        thr_can_rx   = common::create_rt_thread(logica_sistema::thread_can_rx,
                            &ctx, common::priorities::CAN_RX);
        LOG_INFO("MAIN", "Hilo CAN_RX    [prio=80] creado");

        thr_adc      = common::create_rt_thread(logica_sistema::thread_adc,
                            &ctx, common::priorities::ADC);
        LOG_INFO("MAIN", "Hilo ADC       [prio=70] creado");

        thr_control  = common::create_rt_thread(logica_sistema::thread_control,
                            &ctx, common::priorities::CONTROL);
        LOG_INFO("MAIN", "Hilo CONTROL   [prio=60] creado");

        thr_can_tx   = common::create_rt_thread(logica_sistema::thread_can_tx_motors,
                            &ctx, common::priorities::CAN_TX_MOTORS);
        LOG_INFO("MAIN", "Hilo CAN_TX    [prio=50] creado");

        thr_bms_sup  = common::create_rt_thread(logica_sistema::thread_bms_supervisor,
                            &ctx, common::priorities::BMS_SUPERVISOR);
        LOG_INFO("MAIN", "Hilo BMS_SUP   [prio=40] creado");

        thr_watchdog = common::create_rt_thread(logica_sistema::thread_watchdog,
                            &ctx, common::priorities::WATCHDOG, SCHED_RR);
        LOG_INFO("MAIN", "Hilo WATCHDOG  [prio=20] creado");

    } catch (const std::exception& e) {
        LOG_ERROR("MAIN", std::string("Error creando hilos RT: ") + e.what());
        LOG_WARN("MAIN",  "NOTA: ejecutar con 'sudo ./ecu_atc8110' para permisos RT");
        ctx.running.store(false);
        return 1;
    }

    LOG_INFO("MAIN", "Sistema RT activo. Esperando pedal de freno para arrancar...");
    LOG_INFO("MAIN", "Presiona Ctrl+C para parar.");

    // ── 7. Esperar a que todos los hilos terminen ─────────────────────────
    pthread_join(thr_can_rx,   nullptr);
    pthread_join(thr_adc,      nullptr);
    pthread_join(thr_control,  nullptr);
    pthread_join(thr_can_tx,   nullptr);
    pthread_join(thr_bms_sup,  nullptr);
    pthread_join(thr_watchdog, nullptr);

    // ── 8. Parada ordenada del hardware ───────────────────────────────────
    LOG_INFO("MAIN", "Parando hardware...");
    if (ctx.actuador) {
        ctx.actuador->write_output("ENABLE", 0.0);
        ctx.actuador->stop();
    }
    ctx.sensores.stop();
    ctx.can_motors.stop();
    ctx.can_bms.stop();

    // ── 9. Parada del sistema de errores ─────────────────────────────────
    LOG_INFO("MAIN", "Parando sistema de errores...");
    ecu::g_error_publisher.stop();

    LOG_INFO("MAIN", "ECU ATC8110 apagado correctamente.");
    common::Logger::Instance().Shutdown();
    return 0;
}
