#pragma once

#include "socketcan_interface.hpp"
#include "can_bms_handler.hpp"
#include "can_protocol.hpp"
#include "../common/interfaces.hpp"
#include "../common/types.hpp"

#include <string>
#include <memory>

namespace comunicacion_can {

class CanManager : public common::ILifecycle {
public:
    explicit CanManager(std::string iface) 
        : driver_{std::move(iface)}, bms_handler_{std::make_unique<BmsCanHandler>()} {}

    bool start() override
    {
        return driver_.start();
    }

    void stop() override
    {
        driver_.stop();
    }

    // ========================================================================
    // Heartbeat y Supervisor
    // ========================================================================
    
    void publish_heartbeat()
    {
        common::CanFrame hb{ID_SUPERVISOR_HB, {0xAA, 0x55, 0x01}, false};
        driver_.send(hb);
    }

    // ========================================================================
    // Batería (BMS)
    // ========================================================================
    
    void publish_battery(const common::BatteryState &bat)
    {
        // Enviar estado básico de batería al supervisor
        common::CanFrame frame{ID_CAN_BMS, {}, false};
        frame.payload = {
            static_cast<uint8_t>(bat.state_of_charge),
            static_cast<uint8_t>(bat.alarm_level),
            static_cast<uint8_t>(bat.num_cells_detected),
        };
        driver_.send(frame);
    }

    // ========================================================================
    // Motores
    // ========================================================================
    
    /**
     * Solicita telemetría de un motor específico
     * @param motor_id ID del motor (1-4)
     * @param msg_type Tipo de mensaje a solicitar
     */
    bool request_motor_telemetry(uint8_t motor_id, MotorMessageType msg_type)
    {
        if (motor_id < 1 || motor_id > 4) {
            LOG_ERROR("CanManager", "ID de motor inválido: " + std::to_string(motor_id));
            return false;
        }

        // Determinar ID CAN según el motor
        uint32_t can_id = ID_MOTOR_1_CMD + (motor_id - 1);
        
        // Construir mensaje
        common::CanFrame frame{can_id, {}, false};
        uint8_t data[8] = {0};
        build_motor_request(data, msg_type);
        
        uint8_t dlc = get_motor_message_length(msg_type);
        frame.payload.assign(data, data + dlc);
        
        return driver_.send(frame);
    }

    /**
     * Envía comando de aceleración/frenado a un motor
     * @param motor_id ID del motor (1-4)
     * @param throttle Valor de aceleración (0-255)
     * @param brake Valor de frenado (0-255)
     */
    bool send_motor_command(uint8_t motor_id, uint8_t throttle, uint8_t brake)
    {
        if (motor_id < 1 || motor_id > 4) {
            return false;
        }

        uint32_t can_id = ID_MOTOR_1_CMD + (motor_id - 1);
        common::CanFrame frame{can_id, {throttle, brake}, false};
        
        return driver_.send(frame);
    }

    // ========================================================================
    // Recepción de Mensajes
    // ========================================================================
    
    void process_rx(common::SystemSnapshot &snapshot)
    {
        auto frame = driver_.receive();
        if (!frame) {
            return;
        }

        // Procesar según el ID del mensaje
        if (frame->id == ID_CAN_BMS) {
            // Mensaje del BMS
            bms_handler_->process_message(*frame, snapshot.battery);
        }
        else if (frame->id >= ID_MOTOR_1_RESP && frame->id <= ID_MOTOR_4_RESP) {
            // Respuesta de motor
            process_motor_response(*frame, snapshot);
        }
        else if (frame->id == ID_SUPERVISOR_HB || frame->id == ID_SUPERVISOR_CMD) {
            // Mensaje del supervisor
            process_supervisor_message(*frame, snapshot);
        }
        else {
            LOG_DEBUG("CanManager", "Mensaje CAN desconocido: ID=0x" + std::to_string(frame->id));
        }
    }

    // Configurar filtro para recibir solo mensajes relevantes
    bool configure_filters()
    {
        // Por ahora, recibir todos los mensajes
        // En producción, configurar filtros específicos para cada ID
        return true;
    }

private:
    void process_motor_response(const common::CanFrame& frame, common::SystemSnapshot& snapshot)
    {
        // Determinar qué motor envió la respuesta
        uint8_t motor_idx = frame.id - ID_MOTOR_1_RESP;
        if (motor_idx >= 4 || frame.payload.empty()) {
            return;
        }

        auto& motor = snapshot.motors[motor_idx];
        uint8_t cmd = frame.payload[0];

        // Decodificar según tipo de respuesta CCP
        switch (cmd) {
            case CCP_FLASH_READ: // Respuesta a lectura de flash
                if (frame.payload.size() >= 3) {
                    uint8_t addr = frame.payload[1];
                    if (addr == INFO_MODULE_NAME) {
                        // Modelo del controlador (8 bytes ASCII)
                        LOG_INFO("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                                " modelo detectado");
                    } else if (addr == INFO_SOFTWARE_VER) {
                        // Versión de software
                        LOG_INFO("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                                " versión SW detectada");
                    }
                }
                break;
                
            case CCP_MONITOR1: // PWM, temperaturas
                if (frame.payload.size() >= 5) {
                    motor.motor_temp_c = static_cast<double>(frame.payload[1]);
                    motor.inverter_temp_c = static_cast<double>(frame.payload[2]);
                    // PWM duty cycle en payload[3-4] si es necesario
                    LOG_DEBUG("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                             " temp: motor=" + std::to_string(motor.motor_temp_c) + 
                             "°C, inverter=" + std::to_string(motor.inverter_temp_c) + "°C");
                }
                break;
                
            case CCP_MONITOR2: // RPM, corriente %
                if (frame.payload.size() >= 5) {
                    // RPM en bytes 1-2 (big endian)
                    uint16_t rpm = (static_cast<uint16_t>(frame.payload[1]) << 8) | 
                                   frame.payload[2];
                    motor.rpm = static_cast<double>(rpm);
                    
                    // Corriente en porcentaje (byte 3)
                    motor.current_a = static_cast<double>(frame.payload[3]);
                    
                    LOG_DEBUG("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                             " RPM=" + std::to_string(motor.rpm) + 
                             ", I=" + std::to_string(motor.current_a) + "%");
                }
                break;
                
            case CCP_A2D_BATCH_READ1: // Batch 1: freno, acel, voltajes
                if (frame.payload.size() >= 8) {
                    // Decodificar valores A/D si es necesario
                    LOG_DEBUG("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                             " A/D batch 1 recibido");
                }
                break;
                
            case CCP_A2D_BATCH_READ2: // Batch 2: corrientes y voltajes fases
                if (frame.payload.size() >= 8) {
                    // Decodificar valores A/D si es necesario
                    LOG_DEBUG("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                             " A/D batch 2 recibido");
                }
                break;
                
            case COM_SW_ACC: // Estado switch acelerador
            case COM_SW_BRK: // Estado switch freno
            case COM_SW_REV: // Estado switch reversa
                if (frame.payload.size() >= 2) {
                    bool switch_state = (frame.payload[1] != 0);
                    LOG_DEBUG("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                             " switch cmd=0x" + std::to_string(cmd) + 
                             " state=" + std::to_string(switch_state));
                }
                break;
                
            default:
                LOG_DEBUG("CanManager", "Motor " + std::to_string(motor_idx + 1) + 
                         " respuesta desconocida: cmd=0x" + std::to_string(cmd));
                break;
        }
        
        // Motor respondió, está activo
        motor.enabled = true;
    }

    void process_supervisor_message(const common::CanFrame& frame, common::SystemSnapshot& snapshot)
    {
        if (frame.id == ID_SUPERVISOR_HB) {
            // Heartbeat del supervisor recibido
            snapshot.faults.warning = false;
            LOG_DEBUG("CanManager", "Heartbeat supervisor recibido");
        }
        else if (frame.id == ID_SUPERVISOR_CMD) {
            // Comando del supervisor
            if (!frame.payload.empty()) {
                uint8_t cmd = frame.payload[0];
                LOG_INFO("CanManager", "Comando supervisor: 0x" + std::to_string(cmd));
            }
        }
    }

    SocketCanInterface driver_;
    std::unique_ptr<BmsCanHandler> bms_handler_;
};

} // namespace comunicacion_can
