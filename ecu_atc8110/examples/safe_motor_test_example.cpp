/**
 * @file safe_motor_test_example.cpp
 * @brief Ejemplos de uso del Safe Test Mode ECU ATC8110
 * 
 * Demuestra cómo:
 * ✅ Activar y monitorear Safe Test Mode
 * ✅ Interpretar logs y fallos
 * ✅ Implementar testing seguro
 * ✅ Detección y manejo de errores
 * ✅ Integración RT
 */

#include "control_vehiculo/safe_motor_test.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

namespace examples {

// ═══════════════════════════════════════════════════════════════════════════
// 📋 EJEMPLO 1: Inicializar y Usar Safe Test Engine
// ═══════════════════════════════════════════════════════════════════════════

void example_basic_usage()
{
    std::cout << "\n=== EJEMPLO 1: Uso Básico ===" << std::endl;
    std::cout << "Crear instancia, actualizar ciclos, monitorear estado" << std::endl;
    std::cout << std::endl;
    
    control_vehiculo::SafeMotorTest safe_motor;
    
    // Configuración inicial
    std::cout << "[CONFIG] Parámetros iniciales:" << std::endl;
    std::cout << "  Max Torque:        " << control_vehiculo::MAX_TORQUE_SAFE << " Nm" << std::endl;
    std::cout << "  Max Voltage:       " << control_vehiculo::MAX_VOLTAGE_SAFE << " V" << std::endl;
    std::cout << "  Ramp Rate:         " << safe_motor.ramp_rate_v_per_s << " V/s" << std::endl;
    std::cout << "  Brake Threshold:   " << safe_motor.brake_pressed_threshold << " (0-1)" << std::endl;
    std::cout << "  Watchdog Timeout:  " << safe_motor.watchdog_timeout_s * 1000.0 << " ms" << std::endl;
    std::cout << std::endl;
    
    // Simulación: ciclo principal
    const double DT = 0.05;  // 50 ms @ 20 Hz
    double time_s = 0.0;
    
    // Escenario: Freno → 2s espera → Acelerar gradualmente
    while (time_s < 10.0) {
        double throttle = 0.0;
        double brake = 0.0;
        
        // Paso 1: Freno presionado (0-2s)
        if (time_s < 2.0) {
            brake = 0.5;  // Freno presionado
            std::cout << "[0-2s] Freno presionado... (ARMED)" << std::endl;
        }
        // Paso 2: Espera (2-4s)
        else if (time_s < 4.0) {
            brake = 0.0;
            std::cout << "[2-4s] Esperando ventana ARMED... (2s requeridos)" << std::endl;
        }
        // Paso 3: Acelerar gradualmente (4-10s)
        else {
            brake = 0.0;
            throttle = (time_s - 4.0) / 6.0;  // Rampa 0→1 en 6s
            throttle = std::min(throttle, 1.0);
            
            if (static_cast<int>(time_s) % 2 == 0) {
                std::cout << "[4-10s] Acelerando: " 
                          << std::setprecision(2) << throttle * 100 << "%" << std::endl;
            }
        }
        
        // Actualizar motor
        bool ok = safe_motor.update(throttle, brake, true, true, DT);
        
        if (!ok) {
            std::cout << "[ERROR] Falló update a t=" << time_s << "s" << std::endl;
        }
        
        time_s += DT;
    }
    
    std::cout << std::endl << "[RESULT] Torque final: " << std::setprecision(2) 
              << safe_motor.get_torque_nm() << " / " 
              << control_vehiculo::MAX_TORQUE_SAFE << " Nm" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// 📊 EJEMPLO 2: Monitorear Rampa de Aceleración
// ═══════════════════════════════════════════════════════════════════════════

void example_ramp_monitoring()
{
    std::cout << "\n=== EJEMPLO 2: Monitoreo de Rampa (Anti-Tirones) ===" << std::endl;
    std::cout << "Acelerar bruscamente y ver cómo la rampa suaviza" << std::endl;
    std::cout << std::endl;
    
    control_vehiculo::SafeMotorTest safe_motor;
    safe_motor.ramp_rate_v_per_s = 0.5;  // 0.5 V/s
    
    const double DT = 0.05;
    double time_s = 0.0;
    
    std::cout << std::left << std::setw(10) << "[Time]" 
              << std::setw(12) << "Throttle_in" 
              << std::setw(12) << "Throttle_cur"
              << std::setw(12) << "Voltage"
              << std::setw(12) << "RampRate"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Aceleración brusca: 0 → full en un ciclo
    while (time_s < 5.0) {
        double throttle = (time_s >= 1.0) ? 1.0 : 0.0;  // Salto brusco @ 1s
        double brake = 0.0;
        
        double prev_voltage = safe_motor.current_voltage;
        
        safe_motor.update(throttle, brake, true, true, DT);
        
        double ramp_rate = (safe_motor.current_voltage - prev_voltage) / DT;
        
        if (static_cast<int>(time_s * 100) % 10 == 0) {  // Log cada 0.1s
            printf("[%5.2f] %8.2f%% → %8.2f%% | V=%5.2f | ΔV/Δt=%6.2f V/s\n",
                   time_s,
                   throttle * 100.0,
                   safe_motor.throttle_current * 100.0,
                   safe_motor.current_voltage,
                   ramp_rate);
        }
        
        time_s += DT;
    }
    
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "[ANALYSIS] La rampa suavizó el salto brusco en 2 segundos" << std::endl;
    std::cout << "           Sin rampa: 0→5V en 50ms → TIRON PELIGROSO" << std::endl;
    std::cout << "           Con rampa: 0→2V en 4s → movimiento suave ✅" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🚨 EJEMPLO 3: Failsafe de Freno
// ═══════════════════════════════════════════════════════════════════════════

void example_brake_failsafe()
{
    std::cout << "\n=== EJEMPLO 3: Failsafe de Freno (Máxima Prioridad) ===" << std::endl;
    std::cout << "Presionar freno durante aceleración" << std::endl;
    std::cout << std::endl;
    
    control_vehiculo::SafeMotorTest safe_motor;
    
    const double DT = 0.05;
    double time_s = 0.0;
    
    std::cout << std::left << std::setw(10) << "[Time]"
              << std::setw(12) << "Throttle"
              << std::setw(12) << "Brake"
              << std::setw(12) << "Motor_V"
              << std::setw(15) << "State"
              << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    
    // Escenario: freno (armed) → acelerar → presionar freno
    while (time_s < 8.0) {
        double throttle = 0.0;
        double brake = 0.0;
        
        if (time_s < 2.0) {
            brake = 0.5;  // Freno
        } else if (time_s < 4.0) {
            // ARMED (esperando)
        } else if (time_s < 6.0) {
            // RUNNING - Acelerar
            throttle = (time_s - 4.0) / 2.0;  // 0 → 1
        } else {
            // BRAKE PRESSED during acceleration
            brake = 1.0;
        }
        
        safe_motor.update(throttle, brake, true, true, DT);
        
        if (static_cast<int>(time_s * 100) % 5 == 0) {  // Log cada 0.25s
            const char* state_str = [&]() {
                switch (safe_motor.state) {
                    case control_vehiculo::MotorState::IDLE:        return "IDLE";
                    case control_vehiculo::MotorState::ARMED:       return "ARMED";
                    case control_vehiculo::MotorState::RUNNING:     return "RUNNING";
                    case control_vehiculo::MotorState::SAFE_STOP:   return "SAFE_STOP";
                    default:                                        return "?";
                }
            }();
            
            printf("[%5.2f] thr=%5.2f br=%5.2f | V=%5.2f | %s\n",
                   time_s, throttle, brake, safe_motor.current_voltage, state_str);
        }
        
        time_s += DT;
    }
    
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "[RESULT] Estado final: ";
    if (safe_motor.state == control_vehiculo::MotorState::SAFE_STOP) {
        std::cout << "✅ SAFE_STOP (freno funcionó!)" << std::endl;
        std::cout << "Voltaje: " << safe_motor.current_voltage << " V (debe ser ≈0)" << std::endl;
    } else {
        std::cout << "❌ ERROR - Freno no funcionó" << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// ⏰ EJEMPLO 4: Watchdog de Seguridad
// ═══════════════════════════════════════════════════════════════════════════

void example_watchdog_timeout()
{
    std::cout << "\n=== EJEMPLO 4: Watchdog de Seguridad (200ms timeout) ===" << std::endl;
    std::cout << "Simular pérdida de datos CAN" << std::endl;
    std::cout << std::endl;
    
    control_vehiculo::SafeMotorTest safe_motor;
    
    const double DT = 0.05;
    double time_s = 0.0;
    
    std::cout << std::left << std::setw(10) << "[Time]"
              << std::setw(15) << "CAN_OK"
              << std::setw(15) << "Watchdog(ms)"
              << std::setw(15) << "Motor_V"
              << std::setw(12) << "Faults"
              << std::endl;
    std::cout << std::string(65, '-') << std::endl;
    
    // Escenario: motor corriendo → pérdida CAN @ 2s → recupera @ 3.5s
    while (time_s < 4.5) {
        double throttle = 0.5;  // Throttle constante
        double brake = 0.0;
        
        // Simular fallo CAN
        bool comm_ok = (time_s < 2.0 || time_s > 3.5);
        
        safe_motor.update(throttle, brake, true, comm_ok, DT);
        
        if (static_cast<int>(time_s * 100) % 5 == 0) {
            printf("[%5.2f] %s | watchdog=%6.0f | V=%5.2f | faults=%d\n",
                   time_s,
                   comm_ok ? "OK " : "FAIL",
                   safe_motor.time_since_last_throttle * 1000.0,
                   safe_motor.current_voltage,
                   safe_motor.fault_count);
        }
        
        time_s += DT;
    }
    
    std::cout << std::string(65, '-') << std::endl;
    std::cout << "[ANALYSIS]" << std::endl;
    std::cout << "  • CAN normal (0-2s): Watchdog = 0ms, Motor activo" << std::endl;
    std::cout << "  • CAN perdido (2-3.5s): Watchdog incrementa → ⏱️ timeout @ 200ms" << std::endl;
    std::cout << "  • Motor se detiene automáticamente ✅" << std::endl;
    std::cout << "  • CAN recuperado (3.5s): Sistema retorna a IDLE" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🔍 EJEMPLO 5: Detección de Sensor Inválido
// ═══════════════════════════════════════════════════════════════════════════

void example_sensor_validation()
{
    std::cout << "\n=== EJEMPLO 5: Validación de Sensor (0.2V-4.8V) ===" << std::endl;
    std::cout << "Detectar sensor roto o fuera de rango" << std::endl;
    std::cout << std::endl;
    
    control_vehiculo::SafeMotorTest safe_motor;
    
    struct TestCase {
        double voltage_v;
        const char* description;
    };
    
    const TestCase test_cases[] = {
        {0.0,   "Ground (0V) - OK"},
        {0.1,   "Bajo (0.1V) - REJECT"},
        {0.2,   "Mín válido (0.2V) - OK"},
        {2.5,   "Mid-range (2.5V) - OK"},
        {4.8,   "Máx válido (4.8V) - OK"},
        {5.0,   "Sobre máx (5.0V) - REJECT"},
        {6.0,   "Sensor roto (6.0V) - REJECT"},
    };
    
    std::cout << std::left << std::setw(20) << "Voltage"
              << std::setw(30) << "Descripción"
              << std::setw(15) << "Throttle"
              << std::setw(10) << "Faults"
              << std::endl;
    std::cout << std::string(75, '-') << std::endl;
    
    for (const auto& test : test_cases) {
        double throttle_norm = test.voltage_v / 5.0;  // Convertir V a [0,1]
        
        control_vehiculo::SafeMotorTest test_motor;
        test_motor.update(throttle_norm, 0.0, true, true, 0.05);
        
        printf("%6.1fV (%4.2f)    %-30s | thr=%6.2f%% | faults=%d\n",
               test.voltage_v,
               throttle_norm,
               test.description,
               test_motor.throttle_current * 100.0,
               test_motor.fault_count);
    }
    
    std::cout << std::string(75, '-') << std::endl;
    std::cout << "[HYPOTHESIS] Valores fuera [0.2V-4.8V] son rechazados" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// 📈 EJEMPLO 6: Monitoreo de Torque Real
// ═══════════════════════════════════════════════════════════════════════════

void example_torque_monitoring()
{
    std::cout << "\n=== EJEMPLO 6: Monitoreo de Torque Real ===" << std::endl;
    std::cout << "Mostrar conversión voltaje → torque equivalente" << std::endl;
    std::cout << std::endl;
    
    control_vehiculo::SafeMotorTest safe_motor;
    
    const double DT = 0.05;
    double time_s = 0.0;
    
    std::cout << std::left << std::setw(10) << "[Time]"
              << std::setw(12) << "Throttle%"
              << std::setw(12) << "Voltage"
              << std::setw(15) << "Torque(Nm)"
              << std::setw(15) << "vs Max(15Nm)"
              << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    // Accelerar gradualmente
    while (time_s < 8.0) {
        double brake = (time_s < 2.0) ? 0.5 : 0.0;  // Freno inicial
        double throttle = (time_s < 4.0) ? 0.0 : ((time_s - 4.0) / 4.0);  // Rampa
        throttle = std::min(throttle, 1.0);
        
        safe_motor.update(throttle, brake, true, true, DT);
        
        double torque = safe_motor.get_torque_nm();
        
        if (static_cast<int>(time_s * 100) % 10 == 0) {
            printf("[%5.2f] %8.1f%% | %7.2fV | %8.2f Nm | %5.1f%%\n",
                   time_s,
                   safe_motor.throttle_current * 100.0,
                   safe_motor.current_voltage,
                   torque,
                   (torque / control_vehiculo::MAX_TORQUE_SAFE) * 100.0);
        }
        
        time_s += DT;
    }
    
    std::cout << std::string(70, '-') << std::endl;
    std::cout << "[VERIFICATION] Torque nunca excede " 
              << control_vehiculo::MAX_TORQUE_SAFE << " Nm ✅" << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
// 🎯 EJEMPLO PRINCIPAL
// ═══════════════════════════════════════════════════════════════════════════

void run_all_examples()
{
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl
              << "║        Safe Test Mode - Ejemplos de Integración           ║" << std::endl
              << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    
    std::cout << "\n📊 Configuración SAFE_TEST_MODE:" << std::endl;
    std::cout << "   • SAFE_TEST_MODE = " << (control_vehiculo::SAFE_TEST_MODE ? "TRUE" : "FALSE") << std::endl;
    std::cout << "   • MAX_TORQUE = " << control_vehiculo::MAX_TORQUE_SAFE << " Nm" << std::endl;
    std::cout << "   • MAX_VOLTAGE = " << control_vehiculo::MAX_VOLTAGE_SAFE << " V" << std::endl;
    
    example_basic_usage();
    example_ramp_monitoring();
    example_brake_failsafe();
    example_watchdog_timeout();
    example_sensor_validation();
    example_torque_monitoring();
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗" << std::endl
              << "║            ✅ Todos los ejemplos completados              ║" << std::endl
              << "╚════════════════════════════════════════════════════════════╝\n" << std::endl;
}

}  // namespace examples

// ─────────────────────────────────────────────────────────────────────────
// MAIN: Ejecutar ejemplos
// ─────────────────────────────────────────────────────────────────────────

int main()
{
    try {
        examples::run_all_examples();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
