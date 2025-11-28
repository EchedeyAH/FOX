#pragma once

#include "bms_data_logger.hpp"
#include "motor_data_logger.hpp"
#include "sensor_data_logger.hpp"
#include "imu_data_logger.hpp"
#include "can_traffic_logger.hpp"
#include "error_logger.hpp"
#include "system_data_logger.hpp"
#include "session_manager.hpp"
#include "types.hpp"

#include <memory>
#include <mutex>

namespace common {

/**
 * Gestor centralizado de todos los loggers CSV
 * Coordina el logging de todos los subsistemas del vehículo
 */
class DataLoggerManager {
public:
    struct Config {
        int logging_frequency_hz{100};  // Frecuencia de logging
        bool auto_start{true};          // Iniciar logging automáticamente
    };
    
    explicit DataLoggerManager(const Config& config = Config{});
    ~DataLoggerManager();
    
    // Deshabilitar copia
    DataLoggerManager(const DataLoggerManager&) = delete;
    DataLoggerManager& operator=(const DataLoggerManager&) = delete;
    
    /**
     * Configura el directorio de sesión para todos los loggers
     * Debe llamarse antes de start()
     */
    void set_session_directory(const std::filesystem::path& session_dir);
    
    /**
     * Inicia todos los loggers
     */
    bool start();
    
    /**
     * Detiene todos los loggers
     */
    void stop();
    
    /**
     * Verifica si los loggers están activos
     */
    bool is_active() const { return is_active_; }
    
    /**
     * Registra un snapshot completo del sistema
     * Este es el método principal que distribuye datos a todos los loggers
     */
    void log_snapshot(const SystemSnapshot& snapshot);
    
    /**
     * Métodos individuales para logging de subsistemas específicos
     */
    void log_battery(const BatteryState& state);
    void log_motors(const std::array<MotorState, 4>& motors);
    void log_vehicle(const VehicleState& state);
    void log_imu(const ImuData& data);
    void log_can_frame(const std::string& direction, const std::string& interface, const CanFrame& frame);
    void log_error(ErrorLogger::Severity severity, const std::string& subsystem, 
                  int code, const std::string& message, const std::string& context = "");

private:
    Config config_;
    std::filesystem::path session_directory_;
    bool is_active_{false};
    std::mutex mutex_;
    
    // Loggers especializados
    std::unique_ptr<BmsDataLogger> bms_logger_;
    std::unique_ptr<MotorDataLogger> motor_logger_;
    std::unique_ptr<SensorDataLogger> sensor_logger_;
    std::unique_ptr<ImuDataLogger> imu_logger_;
    std::unique_ptr<CanTrafficLogger> can_logger_;
    std::unique_ptr<ErrorLogger> error_logger_;
    std::unique_ptr<SystemDataLogger> system_logger_;
    
    // Inicializa todos los loggers
    void initialize_loggers();
};

} // namespace common
