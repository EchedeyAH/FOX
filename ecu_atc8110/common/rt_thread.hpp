#pragma once
/**
 * rt_thread.hpp
 * Utilidades para crear hilos POSIX con prioridades Real-Time (SCHED_FIFO)
 * y dormir con período garantizado usando clock_nanosleep TIMER_ABSTIME.
 *
 * Compatible con Ubuntu 18.04 kernel lowlatency/PREEMPT.
 * Requiere: -lpthread -lrt
 */

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include <cerrno>
#include <stdexcept>
#include <string>

namespace common {

/**
 * Bloquea toda la memoria del proceso en RAM.
 * Evita page faults en tiempo real. Llamar una vez en main() antes de crear hilos.
 */
inline void lock_memory()
{
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
        // No lanzar excepción — el sistema puede funcionar sin esto,
        // pero lo avisamos. Requiere CAP_IPC_LOCK o ejecutar con sudo.
        // LOG_WARN: no se puede bloquear memoria (sin privilegios)
    }
}

/**
 * Crea un hilo POSIX con política de planificación real-time.
 *
 * @param func     Función del hilo (void* f(void*))
 * @param arg      Argumento pasado al hilo
 * @param priority Prioridad RT (1-99). Más alto = más prioritario.
 *                 Recomendado: CAN_RX=80, ADC=70, CTRL=60, CAN_TX=50, BMS=40, WDG=20
 * @param policy   SCHED_FIFO (recomendado) o SCHED_RR
 * @return         ID del hilo creado
 * @throws         std::runtime_error si falla la creación
 */
inline pthread_t create_rt_thread(void* (*func)(void*), void* arg,
                                   int priority, int policy = SCHED_FIFO)
{
    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param param{};

    pthread_attr_init(&attr);

    // Política de planificación real-time
    if (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) != 0 ||
        pthread_attr_setschedpolicy(&attr, policy) != 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("RT: No se pudo configurar política de planificación");
    }

    // Prioridad (validar rango)
    int max_prio = sched_get_priority_max(policy);
    int min_prio = sched_get_priority_min(policy);
    if (priority > max_prio) priority = max_prio;
    if (priority < min_prio) priority = min_prio;
    param.sched_priority = priority;

    if (pthread_attr_setschedparam(&attr, &param) != 0) {
        pthread_attr_destroy(&attr);
        throw std::runtime_error("RT: No se pudo asignar prioridad " + std::to_string(priority));
    }

    // Tamaño de stack suficiente para módulos ECU
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024); // 8MB

    int ret = pthread_create(&thread, &attr, func, arg);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        if (ret == EPERM) {
            // Sin privilegios RT → crear hilo normal como fallback
            pthread_create(&thread, nullptr, func, arg);
        } else {
            throw std::runtime_error("RT: pthread_create falló, error=" + std::to_string(ret));
        }
    }

    return thread;
}

/**
 * Duerme hasta el siguiente período absoluto.
 * Usar con CLOCK_MONOTONIC para evitar deriva acumulativa.
 *
 * Uso típico:
 *   struct timespec next;
 *   clock_gettime(CLOCK_MONOTONIC, &next);
 *   while (running) {
 *       do_work();
 *       periodic_sleep(next, 50'000'000); // 50ms
 *   }
 *
 * @param next      [in/out] Tiempo absoluto del próximo despertar (se incrementa)
 * @param period_ns Período en nanosegundos
 */
inline void periodic_sleep(struct timespec& next, long period_ns)
{
    // Dormir hasta el tiempo absoluto next
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

    // Avanzar al siguiente período
    next.tv_nsec += period_ns;
    if (next.tv_nsec >= 1'000'000'000L) {
        next.tv_nsec -= 1'000'000'000L;
        next.tv_sec += 1;
    }
}

/**
 * Constantes de período en nanosegundos para cada hilo.
 */
namespace periods {
    constexpr long CAN_RX_NS         =   1'000'000L;  //   1 ms  — CAN receive
    constexpr long ADC_NS            =  10'000'000L;  //  10 ms  — ADC polling
    constexpr long CONTROL_NS        =  20'000'000L;  //  20 ms  — traction control
    constexpr long CAN_TX_NS         =  50'000'000L;  //  50 ms  — motor commands
    constexpr long BMS_SUPERVISOR_NS = 100'000'000L;  // 100 ms  — BMS + heartbeat
    constexpr long WATCHDOG_NS       = 500'000'000L;  // 500 ms  — fault watchdog
}

/**
 * Prioridades RT para cada hilo (SCHED_FIFO, 1-99).
 * Cuanto mayor, más prioritario.
 */
namespace priorities {
    constexpr int CAN_RX         = 80;
    constexpr int ADC            = 70;
    constexpr int CONTROL        = 60;
    constexpr int CAN_TX_MOTORS  = 50;
    constexpr int BMS_SUPERVISOR = 40;
    constexpr int WATCHDOG       = 20;
}

} // namespace common
