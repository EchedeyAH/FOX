#pragma once

#include <cstdint>
#include <string>

namespace comunicacion_can {

// ============================================================================
// IDs CAN del Sistema
// ============================================================================

// BMS (Battery Management System) - CAN2 @ 500 Kbps
constexpr uint32_t ID_CAN_BMS = 0x04D;

// Motores - Comandos ECU → Motor (CAN1 @ 1 Mbps)
constexpr uint32_t ID_MOTOR_1_CMD = 0x201;
constexpr uint32_t ID_MOTOR_2_CMD = 0x202;
constexpr uint32_t ID_MOTOR_3_CMD = 0x203;
constexpr uint32_t ID_MOTOR_4_CMD = 0x204;

// Motores - Respuestas Motor → ECU (CAN1 @ 1 Mbps)
constexpr uint32_t ID_MOTOR_1_RESP = 0x281;
constexpr uint32_t ID_MOTOR_2_RESP = 0x282;
constexpr uint32_t ID_MOTOR_3_RESP = 0x283;
constexpr uint32_t ID_MOTOR_4_RESP = 0x284;

// Supervisor (CAN1 @ 1 Mbps)
constexpr uint32_t ID_SUPERVISOR_HB = 0x100;    // Heartbeat
constexpr uint32_t ID_SUPERVISOR_CMD = 0x101;   // Comandos

// ============================================================================
// Protocolo BMS - Tipos de Parámetros
// ============================================================================

constexpr char VOLTAJE_T = 'V';
constexpr char TEMPERATURA_T = 'T';
constexpr char ESTADO_T = 'E';
constexpr char ALARMA_T = 'A';

// V2 Protocol Characters
constexpr char VOLTAJE_T_V2 = 'v';
constexpr char TEMPERATURA_T_V2 = 't';
constexpr char ESTADO_T_V2 = '%';
constexpr char STATUS_T_V2 = 's';

constexpr uint8_t NUM_CEL_BAT = 24;  // Número de celdas en el pack

// ============================================================================
// Protocolo Motores - Tipos de Mensajes CCP
// ============================================================================

enum MotorMessageType : uint8_t {
    MSG_TIPO_01 = 1,   // Modelo del controlador
    MSG_TIPO_02 = 2,   // Versión software
    MSG_TIPO_03 = 3,   // Zona muerta inferior acelerador
    MSG_TIPO_04 = 4,   // Zona muerta superior acelerador
    MSG_TIPO_05 = 5,   // Zona muerta inferior freno
    MSG_TIPO_06 = 6,   // Zona muerta superior freno
    MSG_TIPO_07 = 7,   // Conversión A/D batch 1 (freno, acel, voltajes)
    MSG_TIPO_08 = 8,   // Conversión A/D batch 2 (corrientes y voltajes fases)
    MSG_TIPO_09 = 9,   // Monitor 1 (PWM, temperaturas)
    MSG_TIPO_10 = 10,  // Monitor 2 (RPM, corriente %)
    MSG_TIPO_11 = 11,  // Switch acelerador
    MSG_TIPO_12 = 12,  // Switch freno
    MSG_TIPO_13 = 13   // Switch reversa
};

// Comandos CCP (CAN Calibration Protocol)
constexpr uint8_t CCP_FLASH_READ = 0xF4;
constexpr uint8_t CCP_A2D_BATCH_READ1 = 0x01;
constexpr uint8_t CCP_A2D_BATCH_READ2 = 0x02;
constexpr uint8_t CCP_MONITOR1 = 0x03;
constexpr uint8_t CCP_MONITOR2 = 0x04;
constexpr uint8_t CCP_INVALID_COMMAND = 0xFF;

// Parámetros de lectura flash
constexpr uint8_t INFO_MODULE_NAME = 0x00;
constexpr uint8_t INFO_SOFTWARE_VER = 0x08;
constexpr uint8_t CAL_TPS_DEAD_ZONE_LOW = 0x10;
constexpr uint8_t CAL_TPS_DEAD_ZONE_HIGH = 0x11;
constexpr uint8_t CAL_BRAKE_DEAD_ZONE_LOW = 0x12;
constexpr uint8_t CAL_BRAKE_DEAD_ZONE_HIGH = 0x13;

// Comandos de comunicación
constexpr uint8_t COM_SW_ACC = 0x20;
constexpr uint8_t COM_SW_BRK = 0x21;
constexpr uint8_t COM_SW_REV = 0x22;
constexpr uint8_t COM_READING = 0x00;

// ============================================================================
// Niveles de Alarma BMS
// ============================================================================

enum class AlarmLevel : uint8_t {
    NO_ALARMA = 0,
    WARNING = 1,
    ALARMA = 2,
    ALARMA_CRITICA = 3
};

// ============================================================================
// Tipos de Alarma BMS
// ============================================================================

enum class AlarmType : uint8_t {
    PACK_OK = 0,
    CELL_TEMP_HIGH = 1,
    PACK_V_HIGH = 2,
    PACK_V_LOW = 3,
    PACK_I_HIGH = 4,
    CHASIS_CONECT = 5,
    CELL_COM_ERROR = 6,
    SYSTEM_ERROR = 7
};

// ============================================================================
// Constantes de Protocolo
// ============================================================================

constexpr uint8_t NUM_MAX_DIG_INDEX = 2;
constexpr uint8_t NUM_MAX_DIG_VALUE = 5;

// ============================================================================
// Funciones Helper para Construcción de Mensajes
// ============================================================================

// Construye un mensaje CAN para solicitar telemetría de motor
inline void build_motor_request(uint8_t* data, MotorMessageType msg_type) {
    switch (msg_type) {
        case MSG_TIPO_01:
            data[0] = CCP_FLASH_READ;
            data[1] = INFO_MODULE_NAME;
            data[2] = 0x08;
            break;
        case MSG_TIPO_02:
            data[0] = CCP_FLASH_READ;
            data[1] = INFO_SOFTWARE_VER;
            data[2] = 0x02;
            break;
        case MSG_TIPO_03:
            data[0] = CCP_FLASH_READ;
            data[1] = CAL_TPS_DEAD_ZONE_LOW;
            data[2] = 0x01;
            break;
        case MSG_TIPO_04:
            data[0] = CCP_FLASH_READ;
            data[1] = CAL_TPS_DEAD_ZONE_HIGH;
            data[2] = 0x01;
            break;
        case MSG_TIPO_05:
            data[0] = CCP_FLASH_READ;
            data[1] = CAL_BRAKE_DEAD_ZONE_LOW;
            data[2] = 0x01;
            break;
        case MSG_TIPO_06:
            data[0] = CCP_FLASH_READ;
            data[1] = CAL_BRAKE_DEAD_ZONE_HIGH;
            data[2] = 0x01;
            break;
        case MSG_TIPO_07:
            data[0] = CCP_A2D_BATCH_READ1;
            break;
        case MSG_TIPO_08:
            data[0] = CCP_A2D_BATCH_READ2;
            break;
        case MSG_TIPO_09:
            data[0] = CCP_MONITOR1;
            break;
        case MSG_TIPO_10:
            data[0] = CCP_MONITOR2;
            break;
        case MSG_TIPO_11:
            data[0] = COM_SW_ACC;
            data[1] = COM_READING;
            break;
        case MSG_TIPO_12:
            data[0] = COM_SW_BRK;
            data[1] = COM_READING;
            break;
        case MSG_TIPO_13:
            data[0] = COM_SW_REV;
            data[1] = COM_READING;
            break;
    }
}

// Obtiene la longitud de datos para cada tipo de mensaje
inline uint8_t get_motor_message_length(MotorMessageType msg_type) {
    switch (msg_type) {
        case MSG_TIPO_01:
        case MSG_TIPO_02:
        case MSG_TIPO_03:
        case MSG_TIPO_04:
        case MSG_TIPO_05:
        case MSG_TIPO_06:
            return 3;
        case MSG_TIPO_07:
        case MSG_TIPO_08:
        case MSG_TIPO_09:
        case MSG_TIPO_10:
            return 1;
        case MSG_TIPO_11:
        case MSG_TIPO_12:
        case MSG_TIPO_13:
            return 2;
        default:
            return 0;
    }
}

} // namespace comunicacion_can
