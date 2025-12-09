#pragma once

#include "csv_logger_base.hpp"
#include "types.hpp"
#include <sstream>

namespace common {

/**
 * Logger para tráfico CAN (raw)
 * Registra todos los mensajes CAN enviados y recibidos
 */
class CanTrafficLogger : public CsvLoggerBase {
public:
    explicit CanTrafficLogger(const fs::path& session_directory);
    
    // Registra un mensaje CAN
    void log_can_frame(const std::string& direction,  // "TX" o "RX"
                      const std::string& interface,   // "can0" o "can1"
                      const CanFrame& frame);

protected:
    std::string get_csv_header() const override;

private:
    std::string can_frame_to_csv(const std::string& direction,
                                const std::string& interface,
                                const CanFrame& frame) const;
    
    // Convierte payload a string hexadecimal
    std::string payload_to_hex(const std::vector<uint8_t>& payload) const;
};

} // namespace common
