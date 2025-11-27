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
        if (motor_idx >= 4) {
            return;
        }

        // TODO: Implementar decodificación completa de respuestas de motores
        // Por ahora, solo registrar que se recibió
        LOG_DEBUG("CanManager", "Respuesta de motor " + std::to_string(motor_idx + 1));
        
        // Actualizar estado básico del motor
        snapshot.motors[motor_idx].enabled = true;
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
