#pragma once
/**
 * timing_metrics.hpp
 * Métricas de timing para tests decheduler RT
 * 
 * Provee:
 * - Medición de jitter (p50, p95, p99, max)
 * - Conteo de deadline misses
 * - Conteo de overruns
 * - Estadísticas de latencia
 * 
 * Uso:
 *   TimingMetrics metrics;
 *   metrics.record_interval(expected_us, actual_us);
 *   metrics.record_deadline_miss();
 *   
 *   auto jitter = metrics.get_jitter_percentiles();
 *   std::cout << "p95 jitter: " << jitter.p95_us << " us\n";
 */

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <numeric>
#include <vector>

namespace ecu {
namespace testing {

// ============================================================================
// ESTRUCTURAS DE MÉTRICAS
// ============================================================================

/**
 * Percentiles de jitter
 */
struct JitterPercentiles {
    uint64_t p50_us = 0;   // Mediana
    uint64_t p95_us = 0;   // 95th percentile
    uint64_t p99_us = 0;   // 99th percentile
    uint64_t max_us = 0;   // Máximo
    double mean_us = 0;    // Media
    double stddev_us = 0;  // Desviación estándar
};

/**
 * Métricas de deadline
 */
struct DeadlineMetrics {
    uint64_t total_deadlines = 0;
    uint64_t missed_count = 0;
    double miss_rate_percent = 0;
    uint64_t worst_case_latency_us = 0;
};

/**
 * Métricas de overrun
 */
struct OverrunMetrics {
    uint64_t total_periods = 0;
    uint64_t overrun_count = 0;
    double overrun_rate_percent = 0;
    uint64_t worst_case_overrun_us = 0;
};

/**
 * Métricas completas de timing
 */
struct TimingReport {
    JitterPercentiles jitter;
    DeadlineMetrics deadlines;
    OverrunMetrics overruns;
    
    // Timestamp del inicio de la medición
    uint64_t start_timestamp_ms = 0;
    
    // Duración total de la medición
    uint64_t duration_ms = 0;
    
    // Número de muestras
    uint64_t sample_count = 0;
};

// ============================================================================
// TIMING METRICS
// ============================================================================

/**
 * Calcula y almacena métricas de timing para tests de scheduler
 */
class TimingMetrics {
public:
    TimingMetrics() {
        reset();
    }

    /// Resetear todas las métricas
    void reset() {
        intervals_.clear();
        jitter_values_.clear();
        deadline_misses_ = 0;
        overruns_ = 0;
        total_samples_ = 0;
        start_time_us_ = 0;
        last_interval_us_ = 0;
    }

    /// Iniciar medición
    void start() {
        start_time_us_ = current_time_us();
    }

    /// Detener medición
    void stop() {
        // nothing to do
    }

    /// Registrar intervalo medido vs esperado
    void record_interval(uint64_t expected_us, uint64_t actual_us) {
        int64_t diff = static_cast<int64_t>(actual_us) - static_cast<int64_t>(expected_us);
        uint64_t jitter = (diff > 0) ? static_cast<uint64_t>(diff) : static_cast<uint64_t>(-diff);
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        intervals_.push_back({expected_us, actual_us, jitter});
        jitter_values_.push_back(jitter);
        
        last_interval_us_ = actual_us;
        total_samples_++;
    }

    /// Registrar deadline miss
    void record_deadline_miss() {
        deadline_misses_.fetch_add(1, std::memory_order_relaxed);
    }

    /// Registrar overrun
    void record_overrun(uint64_t overrun_us) {
        overruns_.fetch_add(1, std::memory_order_relaxed);
        if (overrun_us > worst_overrun_us_.load()) {
            worst_overrun_us_.store(overrun_us);
        }
    }

    /// Obtener percentiles de jitter
    JitterPercentiles get_jitter_percentiles() const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        JitterPercentiles result;
        
        if (jitter_values_.empty()) {
            return result;
        }

        // Ordenar para percentiles
        std::vector<uint64_t> sorted = jitter_values_;
        std::sort(sorted.begin(), sorted.end());

        size_t n = sorted.size();
        
        result.p50_us = sorted[n * 50 / 100];
        result.p95_us = sorted[n * 95 / 100];
        result.p99_us = sorted[n * 99 / 100];
        result.max_us = sorted.back();

        // Calcular media y desviación estándar
        double sum = 0;
        for (uint64_t v : sorted) {
            sum += static_cast<double>(v);
        }
        result.mean_us = sum / n;

        double variance_sum = 0;
        for (uint64_t v : sorted) {
            double diff = static_cast<double>(v) - result.mean_us;
            variance_sum += diff * diff;
        }
        result.stddev_us = std::sqrt(variance_sum / n);

        return result;
    }

    /// Obtener métricas de deadline
    DeadlineMetrics get_deadline_metrics() const {
        DeadlineMetrics result;
        result.total_deadlines = total_samples_.load();
        result.missed_count = deadline_misses_.load();
        
        if (result.total_deadlines > 0) {
            result.miss_rate_percent = 
                (static_cast<double>(result.missed_count) / result.total_deadlines) * 100.0;
        }
        
        result.worst_case_latency_us = worst_overrun_us_.load();
        
        return result;
    }

    /// Obtener métricas de overrun
    OverrunMetrics get_overrun_metrics() const {
        OverrunMetrics result;
        result.total_periods = total_samples_.load();
        result.overrun_count = overruns_.load();
        
        if (result.total_periods > 0) {
            result.overrun_rate_percent = 
                (static_cast<double>(result.overrun_count) / result.total_periods) * 100.0;
        }
        
        result.worst_case_overrun_us = worst_overrun_us_.load();
        
        return result;
    }

    /// Obtener reporte completo de timing
    TimingReport get_report() const {
        TimingReport report;
        report.jitter = get_jitter_percentiles();
        report.deadlines = get_deadline_metrics();
        report.overruns = get_overrun_metrics();
        report.sample_count = total_samples_.load();
        report.start_timestamp_ms = start_time_us_ / 1000;
        report.duration_ms = last_interval_us_ / 1000;
        
        return report;
    }

    /// Verificar si el jitter excede un umbral
    bool jitter_exceeds(uint64_t threshold_us) const {
        auto jitter = get_jitter_percentiles();
        return jitter.p95_us > threshold_us;
    }

    /// Verificar si hay deadline misses
    bool has_deadline_misses() const {
        return deadline_misses_.load() > 0;
    }

    /// Obtener número de muestras
    uint64_t get_sample_count() const {
        return total_samples_.load();
    }

    /// Obtener último intervalo medido
    uint64_t get_last_interval_us() const {
        return last_interval_us_.load();
    }

private:
    struct Interval {
        uint64_t expected_us;
        uint64_t actual_us;
        uint64_t jitter_us;
    };

    mutable std::mutex mutex_;
    std::vector<Interval> intervals_;
    std::vector<uint64_t> jitter_values_;
    
    std::atomic<uint64_t> deadline_misses_ = 0;
    std::atomic<uint64_t> overruns_ = 0;
    std::atomic<uint64_t> worst_overrun_us_ = 0;
    std::atomic<uint64_t> total_samples_ = 0;
    
    uint64_t start_time_us_ = 0;
    std::atomic<uint64_t> last_interval_us_ = 0;

    uint64_t current_time_us() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }
};

// ============================================================================
// LATENCY METRICS
// ============================================================================

/**
 * Métricas específicas para latencia de eventos
 */
class LatencyMetrics {
public:
    LatencyMetrics() : latencies_(1024) {}

    /// Registrar latencia de evento
    void record_latency(uint64_t latency_us) {
        latencies_.push_back(latency_us);
        
        if (latency_us > max_latency_us_.load()) {
            max_latency_us_.store(latency_us);
        }
        
        if (latency_us < min_latency_us_.load()) {
            min_latency_us_.store(latency_us);
        }
        
        total_latency_us_ += latency_us;
        sample_count_++;
    }

    /// Obtener latencia promedio
    double get_average_latency_us() const {
        if (sample_count_ == 0) return 0;
        return static_cast<double>(total_latency_us_) / sample_count_;
    }

    /// Obtener latencia mínima
    uint64_t get_min_latency_us() const {
        return min_latency_us_.load();
    }

    /// Obtener latencia máxima
    uint64_t get_max_latency_us() const {
        return max_latency_us_.load();
    }

    /// Obtener percentil de latencia
    uint64_t get_percentile_latency(double percentile) const {
        if (latencies_.empty()) return 0;
        
        std::vector<uint64_t> sorted = latencies_;
        std::sort(sorted.begin(), sorted.end());
        
        size_t idx = static_cast<size_t>(sorted.size() * percentile / 100.0);
        return sorted[idx];
    }

    /// Verificar si la latencia excede el umbral
    bool latency_exceeds(uint64_t threshold_us) const {
        return get_percentile_latency(95.0) > threshold_us;
    }

    /// Resetear métricas
    void reset() {
        latencies_.clear();
        total_latency_us_ = 0;
        sample_count_ = 0;
        min_latency_us_ = std::numeric_limits<uint64_t>::max();
        max_latency_us_ = 0;
    }

private:
    std::vector<uint64_t> latencies_;
    std::atomic<uint64_t> total_latency_us_ = 0;
    std::atomic<uint64_t> sample_count_ = 0;
    std::atomic<uint64_t> min_latency_us_ = std::numeric_limits<uint64_t>::max();
    std::atomic<uint64_t> max_latency_us_ = 0;
};

} // namespace testing
} // namespace ecu
