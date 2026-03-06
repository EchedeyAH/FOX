#pragma once
/**
 * clock_provider.hpp
 * Abstracción de tiempo para el framework de tests
 * 
 * Provee:
 * - IClock: Interfaz abstracta de tiempo
 * - SimClock: Tiempo simulado (para backend SIM)
 * - RealClock: Tiempo real del sistema (para backend REAL)
 * 
 * Uso:
 *   std::unique_ptr<IClock> clock = create_clock(is_simulation);
 *   uint64_t now = clock->now_us();
 *   clock->sleep_us(1000);
 */

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>

namespace ecu {
namespace testing {

// ============================================================================
// INTERFAZ ICLOCK
// ============================================================================

/**
 * Interfaz abstracta para proveedor de tiempo
 */
class IClock {
public:
    virtual ~IClock() = default;

    /// Obtener tiempo actual en microsegundos
    virtual uint64_t now_us() const = 0;

    /// Obtener tiempo actual en milisegundos
    virtual uint64_t now_ms() const = 0;

    /// Dormir por specified microseconds
    virtual void sleep_us(uint64_t us) const = 0;

    /// Dormir por specified milliseconds
    virtual void sleep_ms(uint64_t ms) const = 0;

    /// Verificar si es tiempo simulado
    virtual bool is_simulated() const = 0;

    /// Obtener nombre del proveedor
    virtual const char* name() const = 0;
};

// ============================================================================
// REAL CLOCK
// ============================================================================

/**
 * Proveedor de tiempo real del sistema
 * Usa std::chrono::steady_clock
 */
class RealClock : public IClock {
public:
    RealClock() = default;

    uint64_t now_us() const override {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(
            now.time_since_epoch()).count();
    }

    uint64_t now_ms() const override {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }

    void sleep_us(uint64_t us) const override {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

    void sleep_ms(uint64_t ms) const override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    bool is_simulated() const override { return false; }
    const char* name() const override { return "RealClock"; }
};

// ============================================================================
// SIM CLOCK
// ============================================================================

/**
 * Proveedor de tiempo simulado para tests
 * Permite control del tiempo y aceleración de tests
 */
class SimClock : public IClock {
public:
    SimClock() : time_us_(0), paused_(false) {}

    /// Establecer tiempo actual (en microsegundos)
    void set_time(uint64_t us) { time_us_.store(us); }

    /// Obtener tiempo actual
    uint64_t now_us() const override { 
        return time_us_.load(); 
    }

    uint64_t now_ms() const override { 
        return time_us_.load() / 1000; 
    }

    /// Dormir - avanza el tiempo simulado
    void sleep_us(uint64_t us) const override {
        if (!paused_.load()) {
            time_us_.fetch_add(us);
        }
    }

    void sleep_ms(uint64_t ms) const override {
        sleep_us(ms * 1000);
    }

    /// Pausar avance del tiempo
    void pause() { paused_.store(true); }

    /// Reanudar avance del tiempo
    void resume() { paused_.store(false); }

    /// Verificar si está pausado
    bool is_paused() const { return paused_.load(); }

    bool is_simulated() const override { return true; }
    const char* name() const override { return "SimClock"; }

    /// Avanzar tiempo sin dormir (para tests)
    void advance(uint64_t us) {
        time_us_.fetch_add(us);
    }

    /// Avanzar tiempo en milliseconds
    void advance_ms(uint64_t ms) {
        advance(ms * 1000);
    }

private:
    mutable std::atomic<uint64_t> time_us_;
    mutable std::atomic<bool> paused_;
};

// ============================================================================
// CLOCK MANAGER
// ============================================================================

/**
 * Gestor centralizado de tiempo
 * Selecciona el reloj apropiado basado en el backend
 */
class ClockManager {
public:
    ClockManager() : is_sim_(false) {
        real_clock_ = std::make_unique<RealClock>();
        sim_clock_ = std::make_unique<SimClock>();
    }

    /// Inicializar para simulación
    void set_simulation_mode(bool sim) {
        is_sim_ = sim;
        if (sim) {
            sim_clock_->set_time(0);
            sim_clock_->resume();
        }
    }

    /// Obtener referencia al reloj activo
    IClock& get_clock() {
        return is_sim_ ? *sim_clock_ : *real_clock_;
    }

    /// Obtener reloj simulado (para configuraciones especiales)
    SimClock& get_sim_clock() { return *sim_clock_; }

    /// Obtener reloj real
    RealClock& get_real_clock() { return *real_clock_; }

    /// Verificar si está en modo simulación
    bool is_simulation() const { return is_sim_; }

    /// Obtener tiempo actual según el modo
    uint64_t now_us() const {
        return is_sim_ ? sim_clock_->now_us() : real_clock_->now_us();
    }

    uint64_t now_ms() const {
        return is_sim_ ? sim_clock_->now_ms() : real_clock_->now_ms();
    }

    /// Dormir según el modo
    void sleep_us(uint64_t us) const {
        if (is_sim_) {
            sim_clock_->sleep_us(us);
        } else {
            real_clock_->sleep_us(us);
        }
    }

    void sleep_ms(uint64_t ms) const {
        if (is_sim_) {
            sim_clock_->sleep_ms(ms);
        } else {
            real_clock_->sleep_ms(ms);
        }
    }

private:
    bool is_sim_;
    std::unique_ptr<RealClock> real_clock_;
    std::unique_ptr<SimClock> sim_clock_;
};

// ============================================================================
// FACTORY FUNCTION
// ============================================================================

/// Crear reloj según el tipo
inline std::unique_ptr<IClock> create_clock(bool simulation = false) {
    if (simulation) {
        return std::make_unique<SimClock>();
    }
    return std::make_unique<RealClock>();
}

} // namespace testing
} // namespace ecu
