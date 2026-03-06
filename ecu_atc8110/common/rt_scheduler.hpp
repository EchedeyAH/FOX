#pragma once
/**
 * rt_scheduler.hpp
 * Scheduler RT determinista con tick base 50ms y sublotes.
 * 
 * Tabla de tareas:
 *   - 10ms: lectura rápida sensores críticos
 *   - 20ms: control loop  
 *   - 50ms: supervisor + publicación snapshot
 *   - 100ms: housekeeping
 *   - 500ms: watchdog/log
 * 
 * Características:
 *   - next_deadline += period (no sleep(period))
 *   - Medición de jitter y overrun
 *   - Error raise en overruns excesivos
 */

#include <chrono>
#include <array>
#include <atomic>
#include <functional>
#include <cstdint>

#include "error_catalog.hpp"
#include "error_publisher.hpp"

namespace common {

// Períodos en nanosegundos
constexpr int64_t PERIOD_10MS = 10'000'000LL;
constexpr int64_t PERIOD_20MS = 20'000'000LL;
constexpr int64_t PERIOD_50MS = 50'000'000LL;
constexpr int64_t PERIOD_100MS = 100'000'000LL;
constexpr int64_t PERIOD_500MS = 500'000'000LL;

// Límites de overrun
constexpr int MAX_OVERRUN_COUNT = 3;
constexpr int64_t JITTER_WARNING_US = 1000;  // 1ms warning
constexpr int64_t JITTER_CRITICAL_US = 2000; // 2ms critical

/**
 * Estado del scheduler
 */
enum class SchedulerState {
    OK = 0,
    LIMP_MODE = 1,    // Limitación de potencia por errores persistentes
    SAFE_STOP = 2     // Parada segura por errores críticos
};

/**
 * Métricas del scheduler
 */
struct SchedulerMetrics {
    int64_t last_jitter_us = 0;
    int64_t max_jitter_us = 0;
    int64_t avg_jitter_us = 0;
    int overrun_count = 0;
    uint64_t cycle_count = 0;
    SchedulerState state = SchedulerState::OK;
    
    // Factor de limitación de potencia (1.0 = sin límite)
    double power_limit_factor = 1.0;
};

/**
 * Callback de tarea
 */
using TaskCallback = std::function<void()>;

/**
 * Tarea programada
 */
struct ScheduledTask {
    const char* name;
    int64_t period_ns;
    TaskCallback callback;
    int64_t next_deadline_ns = 0;
    int overrun_count = 0;
    bool enabled = true;
};

/**
 *RtScheduler - Scheduler determinista con múltiples períodos
 */
class RtScheduler {
public:
    RtScheduler();
    
    // Añadir tarea programada
    void add_task(const char* name, int64_t period_ns, TaskCallback callback);
    
    // Iniciar scheduler (debe llamarse desde hilo RT)
    void start();
    
    // Parar scheduler
    void stop();
    
    // Ejecutar un ciclo (llamar desde el hilo del scheduler)
    void run_cycle();
    
    // Obtener métricas
    const SchedulerMetrics& get_metrics() const { return metrics_; }
    
    // Obtener estado
    SchedulerState get_state() const { return metrics_.state; }
    
    // Forzar modo degradado
    void enter_limp_mode(double power_factor);
    
    // Recuperar a estado OK
    void exit_limp_mode();
    
    // Forzar parada segura
    void enter_safe_stop();
    
    // Verificar si está运行
    bool is_running() const { return running_.load(); }

private:
    void check_overruns();
    void publish_metrics();
    
    std::array<ScheduledTask, 8> tasks_;
    int task_count_ = 0;
    
    std::atomic<bool> running_;
    SchedulerMetrics metrics_;
    
    // Tiempos
    struct timespec cycle_start_;
    struct timespec next_tick_;
};

/**
 * Implementación inline
 */
inline RtScheduler::RtScheduler() : running_(false) {
    metrics_.state = SchedulerState::OK;
    metrics_.power_limit_factor = 1.0;
}

inline void RtScheduler::add_task(const char* name, int64_t period_ns, TaskCallback callback) {
    if (task_count_ < static_cast<int>(tasks_.size())) {
        tasks_[task_count_++] = {name, period_ns, callback, 0, 0, true};
    }
}

inline void RtScheduler::start() {
    clock_gettime(CLOCK_MONOTONIC, &next_tick_);
    running_.store(true);
    
    // Inicializar deadlines de todas las tareas
    int64_t now_ns = next_tick_.tv_sec * 1'000'000'000LL + next_tick_.tv_nsec;
    for (int i = 0; i < task_count_; ++i) {
        tasks_[i].next_deadline_ns = now_ns + tasks_[i].period_ns;
    }
}

inline void RtScheduler::stop() {
    running_.store(false);
}

inline void RtScheduler::run_cycle() {
    clock_gettime(CLOCK_MONOTONIC, &cycle_start_);
    int64_t now_ns = cycle_start_.tv_sec * 1'000'000'000LL + cycle_start_.tv_nsec;
    
    // Ejecutar todas las tareas cuyo deadline haya pasado
    for (int i = 0; i < task_count_; ++i) {
        auto& task = tasks_[i];
        if (!task.enabled) continue;
        
        if (now_ns >= task.next_deadline_ns) {
            // Ejecutar tarea
            if (task.callback) {
                task.callback();
            }
            
            // Calcular overrun
            int64_t diff_ns = now_ns - task.next_deadline_ns;
            if (diff_ns > 0) {
                task.overrun_count++;
                metrics_.overrun_count++;
                
                int64_t jitter_us = diff_ns / 1000;
                if (jitter_us > metrics_.max_jitter_us) {
                    metrics_.max_jitter_us = jitter_us;
                }
                
                // Publicar error de overrun si excede límite
                if (task.overrun_count >= MAX_OVERRUN_COUNT) {
                    // TODO: raise_error(SCHED_OVERRUN_*) cuando error_system integrado
                    LOG_WARN("SCHED", std::string(task.name) + " overrun: " + std::to_string(jitter_us) + "us");
                }
            }
            
            // Avanzar deadline
            task.next_deadline_ns += task.period_ns;
        }
    }
    
    metrics_.cycle_count++;
    metrics_.last_jitter_us = (now_ns % PERIOD_50MS) / 1000;
    
    // Verificar overruns globally
    check_overruns();
}

inline void RtScheduler::check_overruns() {
    // Si demasiados overruns, entrar en LIMP_MODE
    if (metrics_.overrun_count > 10 && metrics_.state == SchedulerState::OK) {
        enter_limp_mode(0.5); // 50% potencia
        LOG_WARN("SCHED", "Demasiados overruns -> LIMP_MODE");
    }
}

inline void RtScheduler::publish_metrics() {
    // Publicar snapshot de scheduler via HMI
    // TODO: integrar con ErrorPublisher
}

inline void RtScheduler::enter_limp_mode(double power_factor) {
    if (metrics_.state != SchedulerState::SAFE_STOP) {
        metrics_.state = SchedulerState::LIMP_MODE;
        metrics_.power_limit_factor = power_factor;
        
        // Publicar evento
        ecu::ErrorEvent evt;
        evt.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        evt.code = ecu::ErrorCode::CTRL_FSM_ERR;
        evt.level = ecu::ErrorLevel::GRAVE;
        evt.group = ecu::ErrorGroup::CONTROL;
        evt.status = ecu::ErrorStatus::ACTIVO;
        evt.origin = "SCHED";
        evt.description = "LIMP_MODE activated";
        evt.count = 1;
        
        ecu::g_error_publisher.publish_event(evt);
    }
}

inline void RtScheduler::exit_limp_mode() {
    metrics_.state = SchedulerState::OK;
    metrics_.power_limit_factor = 1.0;
    metrics_.overrun_count = 0;
    
    // Publicar resolución
    ecu::ErrorEvent evt;
    evt.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    evt.code = ecu::ErrorCode::CTRL_FSM_ERR;
    evt.level = ecu::ErrorLevel::INFORMATIVO;
    evt.group = ecu::ErrorGroup::CONTROL;
    evt.status = ecu::ErrorStatus::RESUELTO;
    evt.origin = "SCHED";
    evt.description = "LIMP_MODE cleared";
    evt.count = 1;
    
    ecu::g_error_publisher.publish_event(evt);
}

inline void RtScheduler::enter_safe_stop() {
    metrics_.state = SchedulerState::SAFE_STOP;
    metrics_.power_limit_factor = 0.0; // Sin potencia
    
    // Publicar evento crítico
    ecu::ErrorEvent evt;
    evt.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    evt.code = ecu::ErrorCode::ERR_EMERGENCY;
    evt.level = ecu::ErrorLevel::CRITICO;
    evt.group = ecu::ErrorGroup::ERROR;
    evt.status = ecu::ErrorStatus::ACTIVO;
    evt.origin = "SCHED";
    evt.description = "SAFE_STOP activated";
    evt.count = 1;
    
    ecu::g_error_publisher.publish_event(evt);
}

// Instancia global
inline RtScheduler& get_global_scheduler() {
    static RtScheduler scheduler;
    return scheduler;
}

} // namespace common
