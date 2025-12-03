#include "data_logger_manager.hpp"
#include <iostream>

namespace common {

DataLoggerManager::DataLoggerManager(const Config& config)
    : config_(config) {
}

DataLoggerManager::~DataLoggerManager() {
    stop();
}

void DataLoggerManager::set_session_directory(const std::filesystem::path& session_dir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_active_) {
        std::cerr << "Error: No se puede cambiar el directorio de sesión mientras el logging está activo.\n";
        return;
    }
    
    session_directory_ = session_dir;
    initialize_loggers();
}

bool DataLoggerManager::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (is_active_) {
        return true;
    }
    
    if (session_directory_.empty()) {
        std::cerr << "Error: Directorio de sesión no configurado. Llame a set_session_directory() primero.\n";
        return false;
    }
    
    if (!bms_logger_ || !motor_logger_ || !sensor_logger_ || 
        !imu_logger_ || !can_logger_ || !error_logger_ || !system_logger_) {
        std::cerr << "Error: Loggers no inicializados.\n";
        return false;
    }
    
    // Iniciar todos los loggers
    bool success = true;
    success &= bms_logger_->start();
    success &= motor_logger_->start();
    success &= sensor_logger_->start();
    success &= imu_logger_->start();
    success &= can_logger_->start();
    success &= error_logger_->start();
    success &= system_logger_->start();
    
    if (!success) {
        std::cerr << "Error: Algunos loggers no pudieron iniciarse.\n";
        return false;
    }
    
    is_active_ = true;
    std::cout << "Data Logger Manager iniciado correctamente.\n";
    
    // Log de inicio
    error_logger_->log_info("SYSTEM", "Data logging iniciado");
    
    return true;
}

void DataLoggerManager::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_active_) {
        return;
    }
    
    // Log de finalización
    if (error_logger_) {
        error_logger_->log_info("SYSTEM", "Data logging detenido");
    }
    
    // Detener todos los loggers
    if (bms_logger_) bms_logger_->stop();
    if (motor_logger_) motor_logger_->stop();
    if (sensor_logger_) sensor_logger_->stop();
    if (imu_logger_) imu_logger_->stop();
    if (can_logger_) can_logger_->stop();
    if (error_logger_) error_logger_->stop();
    if (system_logger_) system_logger_->stop();
    
    is_active_ = false;
    std::cout << "Data Logger Manager detenido.\n";
}

void DataLoggerManager::log_snapshot(const SystemSnapshot& snapshot) {
    if (!is_active_) {
        return;
    }
    
    // Distribuir datos a cada logger especializado
    log_battery(snapshot.battery);
    log_motors(snapshot.motors);
    log_vehicle(snapshot.vehicle);
    log_imu(snapshot.imu);
    system_logger_->log_system_snapshot(snapshot.power, snapshot.faults);
}

void DataLoggerManager::log_battery(const BatteryState& state) {
    if (is_active_ && bms_logger_) {
        bms_logger_->log_battery_state(state);
    }
}

void DataLoggerManager::log_motors(const std::array<MotorState, 4>& motors) {
    if (is_active_ && motor_logger_) {
        motor_logger_->log_all_motors(motors);
    }
}

void DataLoggerManager::log_vehicle(const VehicleState& state) {
    if (is_active_ && sensor_logger_) {
        sensor_logger_->log_vehicle_state(state);
    }
}

void DataLoggerManager::log_imu(const ImuData& data) {
    if (is_active_ && imu_logger_) {
        imu_logger_->log_imu_data(data);
    }
}

void DataLoggerManager::log_can_frame(const std::string& direction, 
                                     const std::string& interface, 
                                     const CanFrame& frame) {
    if (is_active_ && can_logger_) {
        can_logger_->log_can_frame(direction, interface, frame);
    }
}

void DataLoggerManager::log_error(ErrorLogger::Severity severity, 
                                 const std::string& subsystem,
                                 int code, 
                                 const std::string& message, 
                                 const std::string& context) {
    if (is_active_ && error_logger_) {
        error_logger_->log_error(severity, subsystem, code, message, context);
    }
}

void DataLoggerManager::initialize_loggers() {
    bms_logger_ = std::make_unique<BmsDataLogger>(session_directory_);
    motor_logger_ = std::make_unique<MotorDataLogger>(session_directory_);
    sensor_logger_ = std::make_unique<SensorDataLogger>(session_directory_);
    imu_logger_ = std::make_unique<ImuDataLogger>(session_directory_);
    can_logger_ = std::make_unique<CanTrafficLogger>(session_directory_);
    error_logger_ = std::make_unique<ErrorLogger>(session_directory_);
    system_logger_ = std::make_unique<SystemDataLogger>(session_directory_);
    
    std::cout << "Loggers inicializados para directorio: " << session_directory_ << "\n";
}

} // namespace common
