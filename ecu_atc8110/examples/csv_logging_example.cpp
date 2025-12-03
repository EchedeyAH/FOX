#include "common/session_manager.hpp"
#include "common/data_logger_manager.hpp"
#include "interfaces/pilot_interface.hpp"
#include "common/types.hpp"

#include <iostream>
#include <thread>
#include <chrono>

/**
 * Ejemplo de uso del sistema de logging CSV
 * 
 * Este ejemplo muestra cómo integrar el sistema de logging en la aplicación principal
 */

int main() {
    std::cout << "===========================================\n";
    std::cout << "  Sistema de Logging CSV - Ejemplo de Uso\n";
    std::cout << "===========================================\n\n";
    
    // 1. Crear interfaz de piloto (para integración con Qt/PySide)
    interfaces::PilotInterface pilot_interface;
    
    // 2. Iniciar sesión con nombre de piloto
    std::string pilot_name = "Juan Perez";
    std::string notes = "Prueba del sistema de logging";
    std::string conditions = "Seco, 22°C";
    
    std::cout << "Iniciando sesión para piloto: " << pilot_name << "\n";
    if (!pilot_interface.start_session(pilot_name, notes, conditions)) {
        std::cerr << "Error al iniciar sesión\n";
        return 1;
    }
    
    // 3. Obtener directorio de sesión
    std::string session_dir = pilot_interface.get_session_directory();
    std::cout << "Directorio de sesión: " << session_dir << "\n\n";
    
    // 4. Crear y configurar el gestor de loggers
    common::DataLoggerManager logger_manager;
    logger_manager.set_session_directory(session_dir);
    
    // 5. Iniciar logging
    if (!logger_manager.start()) {
        std::cerr << "Error al iniciar loggers\n";
        return 1;
    }
    
    std::cout << "Sistema de logging iniciado correctamente\n";
    std::cout << "Generando datos de ejemplo...\n\n";
    
    // 6. Simular datos del vehículo y registrarlos
    for (int i = 0; i < 10; ++i) {
        // Crear snapshot del sistema
        common::SystemSnapshot snapshot;
        
        // Datos de batería
        snapshot.battery.pack_voltage_mv = 48000 + (i * 100);
        snapshot.battery.pack_current_ma = 15000 + (i * 50);
        snapshot.battery.state_of_charge = 85.0 - (i * 0.5);
        snapshot.battery.communication_ok = true;
        snapshot.battery.bms_error = false;
        
        // Llenar voltajes de celdas (24 celdas)
        for (int cell = 0; cell < 24; ++cell) {
            snapshot.battery.cell_voltages_mv[cell] = 2000 + cell * 10 + i;
            snapshot.battery.cell_temperatures_c[cell] = 25 + cell;
        }
        
        // Datos de motores (4 motores)
        for (int motor = 0; motor < 4; ++motor) {
            snapshot.motors[motor].label = "Motor_" + std::to_string(motor);
            snapshot.motors[motor].rpm = 1000 + (motor * 100) + (i * 50);
            snapshot.motors[motor].torque_nm = 50.0 + (motor * 5) + i;
            snapshot.motors[motor].inverter_temp_c = 45.0 + motor + (i * 0.5);
            snapshot.motors[motor].motor_temp_c = 55.0 + motor + (i * 0.5);
            snapshot.motors[motor].enabled = true;
        }
        
        // Datos de sensores
        snapshot.vehicle.accelerator = 0.5 + (i * 0.05);
        snapshot.vehicle.brake = 0.0;
        snapshot.vehicle.steering = -0.2 + (i * 0.01);
        snapshot.vehicle.suspension_mm = {10.0, 10.5, 11.0, 11.5};
        snapshot.vehicle.reverse = false;
        
        // Datos de IMU
        snapshot.imu.accel_g = {0.1, 0.2, 9.8};
        snapshot.imu.gyro_rad_s = {0.01, -0.02, 0.005};
        snapshot.imu.euler_rad = {0.0, 0.05, 1.57};
        
        // Datos de potencia
        snapshot.power.demanded_w = 5000 + (i * 100);
        snapshot.power.available_w = 10000;
        snapshot.power.battery_w = 4500 + (i * 100);
        snapshot.power.motors_w = 4000 + (i * 100);
        
        // Fallos
        snapshot.faults.warning = false;
        snapshot.faults.error = false;
        snapshot.faults.critical = false;
        snapshot.faults.description = "";
        
        // Registrar snapshot completo
        logger_manager.log_snapshot(snapshot);
        
        // También se pueden registrar mensajes CAN individuales
        common::CanFrame can_frame;
        can_frame.id = 0x180 + i;
        can_frame.payload = {0xAA, 0x55, static_cast<uint8_t>(i), 0x00, 0xFF};
        logger_manager.log_can_frame("RX", "can0", can_frame);
        
        // Y errores/eventos
        if (i == 5) {
            logger_manager.log_error(common::ErrorLogger::Severity::WARNING, 
                                    "BMS", 101, 
                                    "Temperatura de celda ligeramente elevada");
        }
        
        std::cout << "Snapshot " << (i + 1) << "/10 registrado\n";
        
        // Simular frecuencia de 10 Hz
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\nDatos de ejemplo generados correctamente\n";
    std::cout << "Deteniendo sistema de logging...\n";
    
    // 7. Detener logging
    logger_manager.stop();
    
    // 8. Finalizar sesión
    pilot_interface.end_session();
    
    std::cout << "\n===========================================\n";
    std::cout << "  Sesión finalizada correctamente\n";
    std::cout << "  Revise los archivos CSV en:\n";
    std::cout << "  " << session_dir << "\n";
    std::cout << "===========================================\n";
    
    return 0;
}
