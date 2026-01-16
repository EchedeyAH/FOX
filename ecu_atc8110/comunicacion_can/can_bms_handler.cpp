#include "can_bms_handler.hpp"

#include <cstring>
#include <cstdio>

namespace comunicacion_can {

bool BmsCanHandler::process_message(const common::CanFrame& frame, common::BatteryState& battery_state)
{
    // Verificar que es un mensaje del BMS
    if (frame.id != ID_CAN_BMS) {
        return false;
    }

    // Verificar longitud mínima del mensaje
    if (frame.payload.size() < 8) {
        LOG_ERROR("BMS", "Mensaje demasiado corto: " + std::to_string(frame.payload.size()) + " bytes");
        return false;
    }

    interpret_bms_message(frame.payload.data(), frame.payload.size(), battery_state);
    
    // Marcar comunicación como OK
    battery_state.communication_ok = true;
    battery_state.bms_error = false;
    battery_state.last_update = common::Clock::now();
    
    return true;
}

void BmsCanHandler::interpret_bms_message(const uint8_t* data, size_t length, common::BatteryState& battery_state)
{
    // Extraer índice (2 dígitos hexadecimales en ASCII)
    char c_aux_index[NUM_MAX_DIG_INDEX + 3];
    c_aux_index[0] = '0';
    c_aux_index[1] = 'x';
    c_aux_index[NUM_MAX_DIG_INDEX + 2] = '\0';
    
    for (int i = 0; i < NUM_MAX_DIG_INDEX; i++) {
        c_aux_index[i + 2] = static_cast<char>(data[i]);
    }
    
    int index = static_cast<int>(std::strtol(c_aux_index, nullptr, 0));
    
    // Extraer parámetro (carácter ASCII que define el tipo de mensaje)
    char param = static_cast<char>(data[2]);
    
    // Extraer valor (5 dígitos hexadecimales en ASCII)
    char c_aux_value[NUM_MAX_DIG_VALUE + 3];
    c_aux_value[0] = '0';
    c_aux_value[1] = 'x';
    c_aux_value[NUM_MAX_DIG_VALUE + 2] = '\0';
    
    for (int i = 3; i < (NUM_MAX_DIG_VALUE + 3) && i < static_cast<int>(length); i++) {
        c_aux_value[i - 1] = static_cast<char>(data[i]);
    }
    
    int value = static_cast<int>(std::strtol(c_aux_value, nullptr, 0));
    
    // Procesar según el tipo de parámetro
    switch (param) {
        case VOLTAJE_T:
        case VOLTAJE_T_V2:
            process_voltage_message(index, value, battery_state);
            break;
        case TEMPERATURA_T:
        case TEMPERATURA_T_V2:
            process_temperature_message(index, value, battery_state);
            break;
        case ESTADO_T:
        case ESTADO_T_V2:
        case STATUS_T_V2:
            process_state_message(index, value, battery_state);
            break;
        case ALARMA_T:
            process_alarm_message(index, value, battery_state);
            break;
        default:
            LOG_WARN("BMS", "Tipo de mensaje desconocido: " + std::string(1, param));
    }
}

void BmsCanHandler::process_voltage_message(int index, int value, common::BatteryState& battery_state)
{
    if (index > 0 && index <= NUM_CEL_BAT) {
        battery_state.cell_voltages_mv[index - 1] = static_cast<uint16_t>(value);
        LOG_DEBUG("BMS", "Voltaje celda " + std::to_string(index) + ": " + std::to_string(value) + " mV");
    }
}

void BmsCanHandler::process_temperature_message(int index, int value, common::BatteryState& battery_state)
{
    if (index > 0 && index <= NUM_CEL_BAT) {
        battery_state.cell_temperatures_c[index - 1] = static_cast<uint8_t>(value);
        LOG_DEBUG("BMS", "Temperatura celda " + std::to_string(index) + ": " + std::to_string(value) + " °C");
    }
}

void BmsCanHandler::process_state_message(int index, int value, common::BatteryState& battery_state)
{
    switch (index) {
        case 0:
        case 1:
            // No nos interesa según el código legacy
            break;
        case 2:
            battery_state.num_cells_detected = static_cast<uint8_t>(value);
            LOG_DEBUG("BMS", "Celdas detectadas: " + std::to_string(value));
            break;
        case 3:
            battery_state.temp_avg_c = static_cast<uint8_t>(value);
            break;
        case 4:
            battery_state.voltage_avg_mv = static_cast<uint16_t>(value);
            break;
        case 5:
            battery_state.temp_max_c = static_cast<uint8_t>(value);
            break;
        case 6:
            battery_state.cell_temp_max_id = static_cast<uint8_t>(value);
            break;
        case 7:
            battery_state.voltage_max_mv = static_cast<uint16_t>(value);
            break;
        case 8:
            battery_state.cell_v_max_id = static_cast<uint8_t>(value);
            break;
        case 9:
            battery_state.voltage_min_mv = static_cast<uint16_t>(value);
            break;
        case 10:
            battery_state.cell_v_min_id = static_cast<uint8_t>(value);
            break;
        case 11:
            battery_state.pack_voltage_mv = static_cast<double>(value);
            LOG_DEBUG("BMS", "Voltaje pack: " + std::to_string(value) + " mV");
            break;
        case 12:
            battery_state.pack_current_ma = static_cast<double>(static_cast<int16_t>(value));
            LOG_DEBUG("BMS", "Corriente pack: " + std::to_string(value) + " mA");
            break;
        case 13:
            battery_state.state_of_charge = static_cast<double>(value);
            LOG_DEBUG("BMS", "Estado de carga: " + std::to_string(value) + "%");
            break;
        case 15:
            battery_state.timestamp = static_cast<uint16_t>(value);
            break;
        default:
            break;
    }
}

void BmsCanHandler::process_alarm_message(int index, int value, common::BatteryState& battery_state)
{
    switch (index) {
        case 0:
            // Número de alarmas, no lo usamos
            break;
        case 1:
            // Sin alarma
            battery_state.alarm_level = 0;  // NO_ALARMA
            battery_state.alarm_type = 0;   // PACK_OK
            break;
        case 2:
            if (value != 0) {
                battery_state.alarm_level = 2;  // ALARMA
                battery_state.alarm_type = 1;   // CELL_TEMP_HIGH
                LOG_WARN("BMS", "Alarma: Temperatura de celda alta");
            }
            break;
        case 3:
            if (value != 0) {
                battery_state.alarm_level = 2;  // ALARMA
                battery_state.alarm_type = 2;   // PACK_V_HIGH
                LOG_WARN("BMS", "Alarma: Voltaje pack alto");
            }
            break;
        case 4:
            if (value != 0) {
                battery_state.alarm_level = 1;  // WARNING
                battery_state.alarm_type = 2;   // PACK_V_HIGH
                LOG_WARN("BMS", "Warning: Voltaje pack alto");
            }
            break;
        case 5:
            if (value != 0) {
                battery_state.alarm_level = 2;  // ALARMA
                battery_state.alarm_type = 3;   // PACK_V_LOW
                LOG_WARN("BMS", "Alarma: Voltaje pack bajo");
            }
            break;
        case 6:
            if (value != 0) {
                battery_state.alarm_level = 1;  // WARNING
                battery_state.alarm_type = 3;   // PACK_V_LOW
                LOG_WARN("BMS", "Warning: Voltaje pack bajo");
            }
            break;
        case 7:
            if (value != 0) {
                battery_state.alarm_level = 2;  // ALARMA
                battery_state.alarm_type = 4;   // PACK_I_HIGH
                LOG_WARN("BMS", "Alarma: Corriente pack alta");
            }
            break;
        case 8:
            if (value != 0) {
                battery_state.alarm_level = 1;  // WARNING
                battery_state.alarm_type = 4;   // PACK_I_HIGH
                LOG_WARN("BMS", "Warning: Corriente pack alta");
            }
            break;
        case 9:
            if (value != 0) {
                battery_state.alarm_level = 3;  // ALARMA_CRITICA
                battery_state.alarm_type = 5;   // CHASIS_CONECT
                LOG_ERROR("BMS", "Alarma crítica: Chasis conectado");
            }
            break;
        case 10:
            if (value != 0) {
                battery_state.alarm_level = 3;  // ALARMA_CRITICA
                battery_state.alarm_type = 6;   // CELL_COM_ERROR
                LOG_ERROR("BMS", "Alarma crítica: Error comunicación celda");
            }
            break;
        case 11:
            if (value != 0) {
                battery_state.alarm_level = 3;  // ALARMA_CRITICA
                battery_state.alarm_type = 7;   // SYSTEM_ERROR
                LOG_ERROR("BMS", "Alarma crítica: Error de sistema");
            }
            break;
        case 12:
        default:
            break;
    }
}

} // namespace comunicacion_can
