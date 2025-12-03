#pragma once

#include "can_protocol.hpp"
#include "../common/types.hpp"
#include "../common/logging.hpp"

#include <cstdint>
#include <string>
#include <cstdlib>

namespace comunicacion_can {

/**
 * Handler especializado para comunicación con BMS (Battery Management System)
 * Basado en el código legacy can2_fox.c
 * 
 * Protocolo BMS:
 * - Mensajes con formato: [index][index][param][value][value][value][value][value]
 * - Tipos de parámetros: 'V' (voltaje), 'T' (temperatura), 'E' (estado), 'A' (alarma)
 */
class BmsCanHandler {
public:
    BmsCanHandler() = default;

    /**
     * Procesa un mensaje CAN recibido del BMS
     * @param frame Frame CAN recibido
     * @param battery_state Estado de batería a actualizar
     * @return true si el mensaje fue procesado correctamente
     */
    bool process_message(const common::CanFrame& frame, common::BatteryState& battery_state);

private:
    /**
     * Interpreta el contenido del mensaje CAN según el protocolo BMS
     */
    void interpret_bms_message(const uint8_t* data, size_t length, common::BatteryState& battery_state);
    
    /**
     * Procesa mensajes de voltaje de celdas
     */
    void process_voltage_message(int index, int value, common::BatteryState& battery_state);
    
    /**
     * Procesa mensajes de temperatura de celdas
     */
    void process_temperature_message(int index, int value, common::BatteryState& battery_state);
    
    /**
     * Procesa mensajes de estado del pack
     */
    void process_state_message(int index, int value, common::BatteryState& battery_state);
    
    /**
     * Procesa mensajes de alarmas
     */
    void process_alarm_message(int index, int value, common::BatteryState& battery_state);
};

} // namespace comunicacion_can
